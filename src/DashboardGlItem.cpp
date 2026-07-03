#include "DashboardGlItem.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>

namespace {

struct Vertex {
    QVector2D position;
    QVector4D color;
};

struct ModelTriangle {
    QVector3D a;
    QVector3D b;
    QVector3D c;
    QVector4D color;
};

struct DrawTriangle {
    QVector2D a;
    QVector2D b;
    QVector2D c;
    QVector4D color;
    float depth = 0.0f;
};

struct GltfBufferView {
    int byteOffset = 0;
    int byteStride = 0;
};

struct GltfAccessor {
    int bufferView = -1;
    int byteOffset = 0;
    int componentType = 0;
    int count = 0;
    QString type;
};

static float mixf(float a, float b, float t)
{
    return a + (b - a) * t;
}

static float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(v, hi));
}

static float roadHalfWidth(float y)
{
    const float nearY = -0.92f;
    const float farY = 0.58f;
    const float t = clampf((y - farY) / (nearY - farY), 0.0f, 1.0f);
    return mixf(0.14f, 0.58f, t);
}

static float roadCenterX(float y, float curve)
{
    const float nearY = -0.94f;
    const float farY = 0.58f;
    const float t = clampf((y - nearY) / (farY - nearY), 0.0f, 1.0f);
    return curve * 0.38f * std::pow(t, 1.55f);
}

template <typename T>
static T readValue(const QByteArray &data, int offset)
{
    T value;
    std::memcpy(&value, data.constData() + offset, sizeof(T));
    return value;
}

static int componentSize(int componentType)
{
    switch (componentType) {
    case 5120:
    case 5121:
        return 1;
    case 5122:
    case 5123:
        return 2;
    case 5125:
    case 5126:
        return 4;
    default:
        return 0;
    }
}

static int componentCount(const QString &type)
{
    if (type == QLatin1String("SCALAR")) {
        return 1;
    }
    if (type == QLatin1String("VEC2")) {
        return 2;
    }
    if (type == QLatin1String("VEC3")) {
        return 3;
    }
    if (type == QLatin1String("VEC4")) {
        return 4;
    }
    return 0;
}

static QVector4D materialColor(const QString &name, const QVector4D &fallback)
{
    if (name.contains(QLatin1String("LightBlue"), Qt::CaseInsensitive)) {
        return QVector4D(0.92f, 0.91f, 0.84f, 1.0f);
    }
    if (name.contains(QLatin1String("Window"), Qt::CaseInsensitive)) {
        return QVector4D(0.020f, 0.026f, 0.028f, 1.0f);
    }
    if (name.contains(QLatin1String("Black"), Qt::CaseInsensitive)) {
        return QVector4D(0.010f, 0.010f, 0.010f, 1.0f);
    }
    if (name.contains(QLatin1String("Grey"), Qt::CaseInsensitive)) {
        return QVector4D(0.190f, 0.190f, 0.180f, 1.0f);
    }
    if (name.contains(QLatin1String("Head"), Qt::CaseInsensitive)) {
        return QVector4D(0.95f, 0.54f, 0.22f, 1.0f);
    }
    if (name.contains(QLatin1String("Tail"), Qt::CaseInsensitive)) {
        return QVector4D(0.92f, 0.04f, 0.07f, 1.0f);
    }
    if (name.contains(QLatin1String("Material"), Qt::CaseInsensitive)) {
        return QVector4D(0.82f, 0.82f, 0.76f, 1.0f);
    }
    return fallback;
}

static QVector3D readVec3(const QByteArray &bin,
                          const std::vector<GltfBufferView> &views,
                          const GltfAccessor &accessor,
                          int index)
{
    const GltfBufferView &view = views[static_cast<std::size_t>(accessor.bufferView)];
    const int packedStride = componentSize(accessor.componentType) * componentCount(accessor.type);
    const int stride = view.byteStride > 0 ? view.byteStride : packedStride;
    const int offset = view.byteOffset + accessor.byteOffset + index * stride;

    return QVector3D(readValue<float>(bin, offset),
                     readValue<float>(bin, offset + 4),
                     readValue<float>(bin, offset + 8));
}

static quint32 readIndex(const QByteArray &bin,
                         const std::vector<GltfBufferView> &views,
                         const GltfAccessor &accessor,
                         int index)
{
    const GltfBufferView &view = views[static_cast<std::size_t>(accessor.bufferView)];
    const int packedStride = componentSize(accessor.componentType);
    const int stride = view.byteStride > 0 ? view.byteStride : packedStride;
    const int offset = view.byteOffset + accessor.byteOffset + index * stride;

    switch (accessor.componentType) {
    case 5121:
        return static_cast<quint8>(readValue<quint8>(bin, offset));
    case 5123:
        return readValue<quint16>(bin, offset);
    case 5125:
        return readValue<quint32>(bin, offset);
    default:
        return static_cast<quint32>(index);
    }
}

static std::vector<ModelTriangle> loadVehicleModel()
{
    QFile file(QStringLiteral(":/assets/models/quaternius-white-sedan.glb"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QByteArray data = file.readAll();
    if (data.size() < 20 || std::memcmp(data.constData(), "glTF", 4) != 0) {
        return {};
    }

    QByteArray jsonBytes;
    QByteArray bin;
    int offset = 12;
    while (offset + 8 <= data.size()) {
        const quint32 chunkLength = readValue<quint32>(data, offset);
        const quint32 chunkType = readValue<quint32>(data, offset + 4);
        offset += 8;
        if (offset + static_cast<int>(chunkLength) > data.size()) {
            return {};
        }

        const QByteArray chunk = data.mid(offset, static_cast<int>(chunkLength));
        offset += static_cast<int>(chunkLength);

        if (chunkType == 0x4E4F534A) {
            jsonBytes = chunk;
        } else if (chunkType == 0x004E4942) {
            bin = chunk;
        }
    }

    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    if (!doc.isObject() || bin.isEmpty()) {
        return {};
    }

    const QJsonObject root = doc.object();
    std::vector<GltfBufferView> views;
    const QJsonArray viewArray = root.value(QStringLiteral("bufferViews")).toArray();
    views.reserve(static_cast<std::size_t>(viewArray.size()));
    for (const QJsonValue &value : viewArray) {
        const QJsonObject object = value.toObject();
        GltfBufferView view;
        view.byteOffset = object.value(QStringLiteral("byteOffset")).toInt(0);
        view.byteStride = object.value(QStringLiteral("byteStride")).toInt(0);
        views.push_back(view);
    }

    std::vector<GltfAccessor> accessors;
    const QJsonArray accessorArray = root.value(QStringLiteral("accessors")).toArray();
    accessors.reserve(static_cast<std::size_t>(accessorArray.size()));
    for (const QJsonValue &value : accessorArray) {
        const QJsonObject object = value.toObject();
        GltfAccessor accessor;
        accessor.bufferView = object.value(QStringLiteral("bufferView")).toInt(-1);
        accessor.byteOffset = object.value(QStringLiteral("byteOffset")).toInt(0);
        accessor.componentType = object.value(QStringLiteral("componentType")).toInt(0);
        accessor.count = object.value(QStringLiteral("count")).toInt(0);
        accessor.type = object.value(QStringLiteral("type")).toString();
        accessors.push_back(accessor);
    }

    std::vector<QVector4D> materialColors;
    const QJsonArray materialArray = root.value(QStringLiteral("materials")).toArray();
    materialColors.reserve(static_cast<std::size_t>(materialArray.size()));
    for (const QJsonValue &value : materialArray) {
        const QJsonObject material = value.toObject();
        QVector4D color(0.82f, 0.84f, 0.86f, 1.0f);
        const QJsonObject pbr = material.value(QStringLiteral("pbrMetallicRoughness")).toObject();
        const QJsonArray factor = pbr.value(QStringLiteral("baseColorFactor")).toArray();
        if (factor.size() == 4) {
            color = QVector4D(static_cast<float>(factor.at(0).toDouble()),
                              static_cast<float>(factor.at(1).toDouble()),
                              static_cast<float>(factor.at(2).toDouble()),
                              static_cast<float>(factor.at(3).toDouble()));
        }
        materialColors.push_back(materialColor(material.value(QStringLiteral("name")).toString(), color));
    }

    std::vector<ModelTriangle> triangles;
    QVector3D minPoint(1.0e9f, 1.0e9f, 1.0e9f);
    QVector3D maxPoint(-1.0e9f, -1.0e9f, -1.0e9f);

    const QJsonArray meshArray = root.value(QStringLiteral("meshes")).toArray();
    for (const QJsonValue &meshValue : meshArray) {
        const QJsonArray primitives = meshValue.toObject().value(QStringLiteral("primitives")).toArray();
        for (const QJsonValue &primitiveValue : primitives) {
            const QJsonObject primitive = primitiveValue.toObject();
            const QJsonObject attributes = primitive.value(QStringLiteral("attributes")).toObject();
            const int positionAccessorIndex = attributes.value(QStringLiteral("POSITION")).toInt(-1);
            const int indexAccessorIndex = primitive.value(QStringLiteral("indices")).toInt(-1);
            if (positionAccessorIndex < 0 || indexAccessorIndex < 0
                || positionAccessorIndex >= static_cast<int>(accessors.size())
                || indexAccessorIndex >= static_cast<int>(accessors.size())) {
                continue;
            }

            const GltfAccessor &positionAccessor = accessors[static_cast<std::size_t>(positionAccessorIndex)];
            const GltfAccessor &indexAccessor = accessors[static_cast<std::size_t>(indexAccessorIndex)];
            if (positionAccessor.bufferView < 0 || indexAccessor.bufferView < 0
                || positionAccessor.bufferView >= static_cast<int>(views.size())
                || indexAccessor.bufferView >= static_cast<int>(views.size())) {
                continue;
            }

            const int materialIndex = primitive.value(QStringLiteral("material")).toInt(-1);
            const QVector4D color = materialIndex >= 0 && materialIndex < static_cast<int>(materialColors.size())
                    ? materialColors[static_cast<std::size_t>(materialIndex)]
                    : QVector4D(0.88f, 0.88f, 0.82f, 1.0f);

            for (int i = 0; i + 2 < indexAccessor.count; i += 3) {
                const int ia = static_cast<int>(readIndex(bin, views, indexAccessor, i));
                const int ib = static_cast<int>(readIndex(bin, views, indexAccessor, i + 1));
                const int ic = static_cast<int>(readIndex(bin, views, indexAccessor, i + 2));
                if (ia >= positionAccessor.count || ib >= positionAccessor.count || ic >= positionAccessor.count) {
                    continue;
                }

                const QVector3D a = readVec3(bin, views, positionAccessor, ia);
                const QVector3D b = readVec3(bin, views, positionAccessor, ib);
                const QVector3D c = readVec3(bin, views, positionAccessor, ic);
                triangles.push_back({a, b, c, color});

                minPoint.setX(std::min(minPoint.x(), std::min(a.x(), std::min(b.x(), c.x()))));
                minPoint.setY(std::min(minPoint.y(), std::min(a.y(), std::min(b.y(), c.y()))));
                minPoint.setZ(std::min(minPoint.z(), std::min(a.z(), std::min(b.z(), c.z()))));
                maxPoint.setX(std::max(maxPoint.x(), std::max(a.x(), std::max(b.x(), c.x()))));
                maxPoint.setY(std::max(maxPoint.y(), std::max(a.y(), std::max(b.y(), c.y()))));
                maxPoint.setZ(std::max(maxPoint.z(), std::max(a.z(), std::max(b.z(), c.z()))));
            }
        }
    }

    if (triangles.empty()) {
        return triangles;
    }

    const QVector3D center((minPoint.x() + maxPoint.x()) * 0.5f,
                           minPoint.y(),
                           (minPoint.z() + maxPoint.z()) * 0.5f);
    for (ModelTriangle &triangle : triangles) {
        triangle.a -= center;
        triangle.b -= center;
        triangle.c -= center;
    }

    return triangles;
}

static void addTriangle(std::vector<Vertex> &v,
                        const QVector2D &a,
                        const QVector2D &b,
                        const QVector2D &c,
                        const QVector4D &color)
{
    v.push_back({a, color});
    v.push_back({b, color});
    v.push_back({c, color});
}

static void addQuad(std::vector<Vertex> &v,
                    const QVector2D &a,
                    const QVector2D &b,
                    const QVector2D &c,
                    const QVector2D &d,
                    const QVector4D &color)
{
    addTriangle(v, a, b, c, color);
    addTriangle(v, a, c, d, color);
}

static void addRoadDash(std::vector<Vertex> &v, float y, float length, float curve, const QVector4D &color)
{
    const float topY = y + length;
    const float bottomHalf = roadHalfWidth(y) * 0.035f;
    const float topHalf = roadHalfWidth(topY) * 0.028f;
    const float bottomX = roadCenterX(y, curve);
    const float topX = roadCenterX(topY, curve);

    addQuad(v,
            QVector2D(bottomX - bottomHalf, y),
            QVector2D(bottomX + bottomHalf, y),
            QVector2D(topX + topHalf, topY),
            QVector2D(topX - topHalf, topY),
            color);
}

static void addRoadCrossLine(std::vector<Vertex> &v, float y, float curve, const QVector4D &color)
{
    const float topY = y + 0.018f;
    const float bottomHalf = roadHalfWidth(y) * 0.90f;
    const float topHalf = roadHalfWidth(topY) * 0.88f;
    const float bottomX = roadCenterX(y, curve);
    const float topX = roadCenterX(topY, curve);

    addQuad(v,
            QVector2D(bottomX - bottomHalf, y),
            QVector2D(bottomX + bottomHalf, y),
            QVector2D(topX + topHalf, topY),
            QVector2D(topX - topHalf, topY),
            color);
}

class RoadRenderer final : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
public:
    RoadRenderer() = default;

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        const DashboardGlItem *view = static_cast<DashboardGlItem *>(item);
        m_speed = view->speed();
        m_distance = view->distance();
        m_curve = view->curve();
    }

    void render() override
    {
        if (!m_initialized) {
            initializeOpenGLFunctions();
            initProgram();
            m_vbo.create();
            m_initialized = true;
        }

        glViewport(0, 0, framebufferObject()->width(), framebufferObject()->height());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        std::vector<Vertex> vertices;
        buildScene(vertices);
        if (vertices.empty()) {
            return;
        }

        m_program.bind();
        m_vbo.bind();
        m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(Vertex)));

        const int posLocation = m_program.attributeLocation("aPosition");
        const int colorLocation = m_program.attributeLocation("aColor");

        m_program.enableAttributeArray(posLocation);
        m_program.enableAttributeArray(colorLocation);
        m_program.setAttributeBuffer(posLocation, GL_FLOAT, offsetof(Vertex, position), 2, sizeof(Vertex));
        m_program.setAttributeBuffer(colorLocation, GL_FLOAT, offsetof(Vertex, color), 4, sizeof(Vertex));

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

        m_program.disableAttributeArray(posLocation);
        m_program.disableAttributeArray(colorLocation);
        m_vbo.release();
        m_program.release();
    }

private:
    void initProgram()
    {
        static const char *vertexShader =
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            "attribute vec2 aPosition;\n"
            "attribute vec4 aColor;\n"
            "varying vec4 vColor;\n"
            "void main() {\n"
            "    gl_Position = vec4(aPosition, 0.0, 1.0);\n"
            "    vColor = aColor;\n"
            "}\n";

        static const char *fragmentShader =
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            "varying vec4 vColor;\n"
            "void main() {\n"
            "    gl_FragColor = vColor;\n"
            "}\n";

        m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
        m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
        m_program.link();
    }

    void buildScene(std::vector<Vertex> &v)
    {
        const QVector4D road(0.34f, 0.34f, 0.28f, 0.82f);
        const QVector4D roadEdge(0.05f, 0.98f, 0.80f, 0.28f);
        const QVector4D dash(0.95f, 0.96f, 0.90f, 0.94f);
        const QVector4D glow(0.0f, 0.0f, 0.0f, 0.24f);

        const float nearY = -0.94f;
        const float farY = 0.58f;
        const int segments = 18;

        for (int i = 0; i < segments; ++i) {
            const float y0 = mixf(nearY, farY, static_cast<float>(i) / segments);
            const float y1 = mixf(nearY, farY, static_cast<float>(i + 1) / segments);
            const float x0 = roadCenterX(y0, static_cast<float>(m_curve));
            const float x1 = roadCenterX(y1, static_cast<float>(m_curve));
            const float h0 = roadHalfWidth(y0);
            const float h1 = roadHalfWidth(y1);

            addQuad(v,
                    QVector2D(x0 - h0 - 0.10f, y0 - 0.03f),
                    QVector2D(x0 + h0 + 0.10f, y0 - 0.03f),
                    QVector2D(x1 + h1 + 0.04f, y1 + 0.02f),
                    QVector2D(x1 - h1 - 0.04f, y1 + 0.02f),
                    glow);

            addQuad(v,
                    QVector2D(x0 - h0, y0),
                    QVector2D(x0 + h0, y0),
                    QVector2D(x1 + h1, y1),
                    QVector2D(x1 - h1, y1),
                    road);

            addQuad(v,
                    QVector2D(x0 - h0, y0),
                    QVector2D(x0 - h0 + 0.018f, y0),
                    QVector2D(x1 - h1 + 0.006f, y1),
                    QVector2D(x1 - h1, y1),
                    roadEdge);
            addQuad(v,
                    QVector2D(x0 + h0 - 0.018f, y0),
                    QVector2D(x0 + h0, y0),
                    QVector2D(x1 + h1, y1),
                    QVector2D(x1 + h1 - 0.006f, y1),
                    roadEdge);
        }

        const float speedFactor = clampf(static_cast<float>(m_speed) / 180.0f, 0.0f, 1.8f);
        const float crossPeriod = 0.12f;
        const float crossOffset = std::fmod(static_cast<float>(m_distance) * 0.020f, crossPeriod);

        for (int i = -1; i < 18; ++i) {
            const float y = nearY + i * crossPeriod - crossOffset;
            if (y < nearY || y > farY) {
                continue;
            }

            const float t = clampf((y - farY) / (nearY - farY), 0.0f, 1.0f);
            const float alpha = mixf(0.055f, 0.140f, t) * (0.75f + speedFactor * 0.18f);
            addRoadCrossLine(v, y, static_cast<float>(m_curve), QVector4D(0.86f, 0.88f, 0.78f, alpha));
        }

        const float dashPeriod = 0.38f;
        const float offset = std::fmod(static_cast<float>(m_distance) * 0.038f, dashPeriod);

        for (int i = -1; i < 9; ++i) {
            const float y = nearY + i * dashPeriod - offset;
            if (y < nearY || y > farY) {
                continue;
            }

            const float t = clampf((y - farY) / (nearY - farY), 0.0f, 1.0f);
            const float length = mixf(0.07f, 0.19f, t) * (0.8f + speedFactor * 0.18f);
            addRoadDash(v, y, length, static_cast<float>(m_curve), dash);
        }

        addVehicle(v);
    }

    QVector2D projectVehiclePoint(const QVector3D &point,
                                  float centerX,
                                  float centerY,
                                  float scale,
                                  float yaw) const
    {
        const float c = std::cos(yaw);
        const float s = std::sin(yaw);
        const float correctedX = -point.x();
        const float correctedY = -point.y() * 1.22f;
        const float x = correctedX * c + point.z() * s;
        const float z = -correctedX * s + point.z() * c;

        return QVector2D(centerX + x * scale,
                         centerY - z * scale * 0.56f - correctedY * scale * 0.80f);
    }

    void addVehicleQuad(std::vector<Vertex> &v,
                        float carX,
                        float carY,
                        float scale,
                        float yaw,
                        const QVector3D &a,
                        const QVector3D &b,
                        const QVector3D &c,
                        const QVector3D &d,
                        const QVector4D &color) const
    {
        addQuad(v,
                projectVehiclePoint(a, carX, carY, scale, yaw),
                projectVehiclePoint(b, carX, carY, scale, yaw),
                projectVehiclePoint(c, carX, carY, scale, yaw),
                projectVehiclePoint(d, carX, carY, scale, yaw),
                color);
    }

    void addVehicleDetails(std::vector<Vertex> &v, float carX, float carY, float scale, float yaw) const
    {
        const QVector4D glass(0.015f, 0.022f, 0.024f, 0.94f);
        const QVector4D glassGlow(0.70f, 0.86f, 0.82f, 0.18f);
        const QVector4D bodyShade(0.68f, 0.67f, 0.60f, 0.34f);
        const QVector4D bodyHighlight(1.0f, 0.98f, 0.86f, 0.42f);
        const QVector4D red(0.95f, 0.02f, 0.06f, 0.95f);
        const QVector4D redGlow(1.0f, 0.08f, 0.10f, 0.26f);
        const QVector4D chrome(0.96f, 0.94f, 0.82f, 0.72f);
        const QVector4D tire(0.005f, 0.005f, 0.005f, 0.58f);
        const QVector4D plate(0.88f, 0.82f, 0.58f, 0.82f);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.38f, 1.075f, -0.62f),
                       QVector3D(0.38f, 1.075f, -0.62f),
                       QVector3D(0.31f, 1.125f, 0.46f),
                       QVector3D(-0.31f, 1.125f, 0.46f),
                       glass);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.31f, 1.128f, -0.42f),
                       QVector3D(0.31f, 1.128f, -0.42f),
                       QVector3D(0.24f, 1.140f, 0.36f),
                       QVector3D(-0.24f, 1.140f, 0.36f),
                       glassGlow);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.55f, 0.78f, -1.42f),
                       QVector3D(0.55f, 0.78f, -1.42f),
                       QVector3D(0.42f, 1.02f, -0.84f),
                       QVector3D(-0.42f, 1.02f, -0.84f),
                       glass);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.46f, 0.94f, -1.12f),
                       QVector3D(0.46f, 0.94f, -1.12f),
                       QVector3D(0.38f, 0.98f, -0.92f),
                       QVector3D(-0.38f, 0.98f, -0.92f),
                       glassGlow);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.72f, 0.35f, -1.43f),
                       QVector3D(0.72f, 0.35f, -1.43f),
                       QVector3D(0.60f, 0.56f, -1.18f),
                       QVector3D(-0.60f, 0.56f, -1.18f),
                       bodyShade);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.64f, 0.58f, -1.47f),
                       QVector3D(-0.24f, 0.58f, -1.49f),
                       QVector3D(-0.20f, 0.68f, -1.39f),
                       QVector3D(-0.60f, 0.69f, -1.37f),
                       red);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(0.24f, 0.58f, -1.49f),
                       QVector3D(0.64f, 0.58f, -1.47f),
                       QVector3D(0.60f, 0.69f, -1.37f),
                       QVector3D(0.20f, 0.68f, -1.39f),
                       red);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.69f, 0.54f, -1.51f),
                       QVector3D(-0.17f, 0.54f, -1.52f),
                       QVector3D(-0.14f, 0.72f, -1.35f),
                       QVector3D(-0.66f, 0.72f, -1.34f),
                       redGlow);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(0.17f, 0.54f, -1.52f),
                       QVector3D(0.69f, 0.54f, -1.51f),
                       QVector3D(0.66f, 0.72f, -1.34f),
                       QVector3D(0.14f, 0.72f, -1.35f),
                       redGlow);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.18f, 0.43f, -1.56f),
                       QVector3D(0.18f, 0.43f, -1.56f),
                       QVector3D(0.16f, 0.53f, -1.49f),
                       QVector3D(-0.16f, 0.53f, -1.49f),
                       plate);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.61f, 0.39f, -1.61f),
                       QVector3D(0.61f, 0.39f, -1.61f),
                       QVector3D(0.55f, 0.43f, -1.55f),
                       QVector3D(-0.55f, 0.43f, -1.55f),
                       chrome);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.76f, 0.10f, -1.02f),
                       QVector3D(-0.60f, 0.10f, -1.02f),
                       QVector3D(-0.58f, 0.42f, -0.82f),
                       QVector3D(-0.75f, 0.42f, -0.82f),
                       tire);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(0.60f, 0.10f, -1.02f),
                       QVector3D(0.76f, 0.10f, -1.02f),
                       QVector3D(0.75f, 0.42f, -0.82f),
                       QVector3D(0.58f, 0.42f, -0.82f),
                       tire);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.74f, 0.13f, 0.91f),
                       QVector3D(-0.59f, 0.13f, 0.91f),
                       QVector3D(-0.57f, 0.43f, 1.12f),
                       QVector3D(-0.73f, 0.43f, 1.12f),
                       tire);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(0.59f, 0.13f, 0.91f),
                       QVector3D(0.74f, 0.13f, 0.91f),
                       QVector3D(0.73f, 0.43f, 1.12f),
                       QVector3D(0.57f, 0.43f, 1.12f),
                       tire);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.74f, 0.76f, -0.56f),
                       QVector3D(-0.60f, 0.76f, -0.54f),
                       QVector3D(-0.57f, 0.86f, -0.34f),
                       QVector3D(-0.76f, 0.84f, -0.36f),
                       glass);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(0.60f, 0.76f, -0.54f),
                       QVector3D(0.74f, 0.76f, -0.56f),
                       QVector3D(0.76f, 0.84f, -0.36f),
                       QVector3D(0.57f, 0.86f, -0.34f),
                       glass);

        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.55f, 0.92f, 0.12f),
                       QVector3D(0.55f, 0.92f, 0.12f),
                       QVector3D(0.50f, 0.96f, 0.22f),
                       QVector3D(-0.50f, 0.96f, 0.22f),
                       bodyHighlight);
        addVehicleQuad(v, carX, carY, scale, yaw,
                       QVector3D(-0.57f, 0.55f, -1.16f),
                       QVector3D(0.57f, 0.55f, -1.16f),
                       QVector3D(0.56f, 0.58f, -1.10f),
                       QVector3D(-0.56f, 0.58f, -1.10f),
                       chrome);
    }

    void addVehicle(std::vector<Vertex> &v)
    {
        if (m_carTriangles.empty()) {
            return;
        }

        const float curve = static_cast<float>(m_curve);
        const float carY = -0.48f;
        const float carX = roadCenterX(carY, curve);
        const float scale = 0.198f;
        const float modelFacingCorrection = 3.14159265f;
        const float yaw = modelFacingCorrection - curve * 15.0f * 3.14159265f / 180.0f;

        const QVector4D shadow(0.0f, 0.0f, 0.0f, 0.26f);
        const float shadowW = scale * 1.58f;
        const float shadowH = scale * 0.48f;
        addQuad(v,
                QVector2D(carX - shadowW, carY - shadowH * 0.25f),
                QVector2D(carX + shadowW, carY - shadowH * 0.25f),
                QVector2D(carX + shadowW * 0.76f, carY + shadowH * 0.52f),
                QVector2D(carX - shadowW * 0.76f, carY + shadowH * 0.52f),
                shadow);

        std::vector<DrawTriangle> projected;
        projected.reserve(m_carTriangles.size());
        for (const ModelTriangle &triangle : m_carTriangles) {
            const QVector2D a = projectVehiclePoint(triangle.a, carX, carY, scale, yaw);
            const QVector2D b = projectVehiclePoint(triangle.b, carX, carY, scale, yaw);
            const QVector2D c = projectVehiclePoint(triangle.c, carX, carY, scale, yaw);

            const QVector3D normal = QVector3D::crossProduct(triangle.b - triangle.a,
                                                             triangle.c - triangle.a).normalized();
            const float light = clampf(0.70f + std::abs(normal.y()) * 0.26f - normal.z() * 0.08f
                                               + std::abs(normal.x()) * 0.06f,
                                       0.48f,
                                       1.22f);
            QVector4D color(triangle.color.x() * light,
                            triangle.color.y() * light,
                            triangle.color.z() * light,
                            triangle.color.w());
            color.setX(clampf(color.x(), 0.0f, 1.0f));
            color.setY(clampf(color.y(), 0.0f, 1.0f));
            color.setZ(clampf(color.z(), 0.0f, 1.0f));

            const float depth = ((triangle.a.z() + triangle.b.z() + triangle.c.z()) / 3.0f)
                    + ((triangle.a.y() + triangle.b.y() + triangle.c.y()) / 3.0f) * 0.18f;
            DrawTriangle drawTriangle;
            drawTriangle.a = a;
            drawTriangle.b = b;
            drawTriangle.c = c;
            drawTriangle.color = color;
            drawTriangle.depth = depth;
            projected.push_back(drawTriangle);
        }

        std::sort(projected.begin(), projected.end(), [](const DrawTriangle &left, const DrawTriangle &right) {
            return left.depth > right.depth;
        });

        for (const DrawTriangle &triangle : projected) {
            addTriangle(v, triangle.a, triangle.b, triangle.c, triangle.color);
        }

        addVehicleDetails(v, carX, carY, scale, yaw);
    }

    bool m_initialized = false;
    qreal m_speed = 0.0;
    qreal m_distance = 0.0;
    qreal m_curve = 0.0;
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    std::vector<ModelTriangle> m_carTriangles = loadVehicleModel();
};

} // namespace

DashboardGlItem::DashboardGlItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer *DashboardGlItem::createRenderer() const
{
    return new RoadRenderer();
}

qreal DashboardGlItem::speed() const
{
    return m_speed;
}

void DashboardGlItem::setSpeed(qreal speed)
{
    if (qFuzzyCompare(m_speed, speed)) {
        return;
    }

    m_speed = speed;
    update();
    emit speedChanged();
}

qreal DashboardGlItem::distance() const
{
    return m_distance;
}

void DashboardGlItem::setDistance(qreal distance)
{
    if (qFuzzyCompare(m_distance, distance)) {
        return;
    }

    m_distance = distance;
    update();
    emit distanceChanged();
}

qreal DashboardGlItem::curve() const
{
    return m_curve;
}

void DashboardGlItem::setCurve(qreal curve)
{
    if (qFuzzyCompare(m_curve, curve)) {
        return;
    }

    m_curve = curve;
    update();
    emit curveChanged();
}
