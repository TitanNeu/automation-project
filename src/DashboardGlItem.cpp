#include "DashboardGlItem.h"

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>
#include <QVector4D>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace {

struct Vertex {
    QVector2D position;
    QVector4D color;
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

        const float dashPeriod = 0.38f;
        const float speedFactor = clampf(static_cast<float>(m_speed) / 180.0f, 0.0f, 1.8f);
        const float offset = std::fmod(static_cast<float>(m_distance) * 0.038f, dashPeriod);

        for (int i = -2; i < 8; ++i) {
            const float y = nearY + i * dashPeriod + offset;
            if (y < nearY || y > farY) {
                continue;
            }

            const float t = clampf((y - farY) / (nearY - farY), 0.0f, 1.0f);
            const float length = mixf(0.07f, 0.19f, t) * (0.8f + speedFactor * 0.18f);
            addRoadDash(v, y, length, static_cast<float>(m_curve), dash);
        }

        // The vehicle is drawn as an anti-aliased QML Canvas overlay. Keeping
        // the road in OpenGL and the detailed icon in QML gives a cleaner HMI
        // result at dashboard scale.
    }

    bool m_initialized = false;
    qreal m_speed = 0.0;
    qreal m_distance = 0.0;
    qreal m_curve = 0.0;
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
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
