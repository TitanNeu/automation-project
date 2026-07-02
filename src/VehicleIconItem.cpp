#include "VehicleIconItem.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

namespace {

static QPointF pt(qreal w, qreal h, qreal x, qreal y)
{
    return QPointF(w * x, h * y);
}

static QRectF rect(qreal w, qreal h, qreal x, qreal y, qreal rw, qreal rh)
{
    return QRectF(w * x, h * y, w * rw, h * rh);
}

static QPainterPath rounded(const QRectF &r, qreal radius)
{
    QPainterPath path;
    path.addRoundedRect(r, radius, radius);
    return path;
}

static QPainterPath bodyPath(qreal w, qreal h)
{
    QPainterPath path;
    path.moveTo(pt(w, h, 0.36, 0.035));
    path.cubicTo(pt(w, h, 0.43, 0.005), pt(w, h, 0.57, 0.005), pt(w, h, 0.64, 0.035));
    path.cubicTo(pt(w, h, 0.77, 0.095), pt(w, h, 0.83, 0.235), pt(w, h, 0.81, 0.430));
    path.cubicTo(pt(w, h, 0.84, 0.585), pt(w, h, 0.85, 0.765), pt(w, h, 0.765, 0.910));
    path.cubicTo(pt(w, h, 0.690, 0.995), pt(w, h, 0.310, 0.995), pt(w, h, 0.235, 0.910));
    path.cubicTo(pt(w, h, 0.150, 0.765), pt(w, h, 0.160, 0.585), pt(w, h, 0.190, 0.430));
    path.cubicTo(pt(w, h, 0.170, 0.235), pt(w, h, 0.230, 0.095), pt(w, h, 0.36, 0.035));
    path.closeSubpath();
    return path;
}

static QPainterPath rearWindowPath(qreal w, qreal h)
{
    QPainterPath path;
    path.moveTo(pt(w, h, 0.315, 0.155));
    path.cubicTo(pt(w, h, 0.390, 0.105), pt(w, h, 0.610, 0.105), pt(w, h, 0.685, 0.155));
    path.lineTo(pt(w, h, 0.665, 0.480));
    path.cubicTo(pt(w, h, 0.600, 0.535), pt(w, h, 0.400, 0.535), pt(w, h, 0.335, 0.480));
    path.closeSubpath();
    return path;
}

static QPainterPath rearHatchPath(qreal w, qreal h)
{
    QPainterPath path;
    path.moveTo(pt(w, h, 0.285, 0.610));
    path.cubicTo(pt(w, h, 0.380, 0.575), pt(w, h, 0.620, 0.575), pt(w, h, 0.715, 0.610));
    path.lineTo(pt(w, h, 0.690, 0.800));
    path.cubicTo(pt(w, h, 0.610, 0.840), pt(w, h, 0.390, 0.840), pt(w, h, 0.310, 0.800));
    path.closeSubpath();
    return path;
}

static QPainterPath tailLightPath(qreal w, qreal h, bool left)
{
    QPainterPath path;
    if (left) {
        path.moveTo(pt(w, h, 0.230, 0.705));
        path.cubicTo(pt(w, h, 0.290, 0.680), pt(w, h, 0.385, 0.690), pt(w, h, 0.440, 0.720));
        path.lineTo(pt(w, h, 0.410, 0.750));
        path.cubicTo(pt(w, h, 0.330, 0.735), pt(w, h, 0.270, 0.735), pt(w, h, 0.215, 0.755));
    } else {
        path.moveTo(pt(w, h, 0.770, 0.705));
        path.cubicTo(pt(w, h, 0.710, 0.680), pt(w, h, 0.615, 0.690), pt(w, h, 0.560, 0.720));
        path.lineTo(pt(w, h, 0.590, 0.750));
        path.cubicTo(pt(w, h, 0.670, 0.735), pt(w, h, 0.730, 0.735), pt(w, h, 0.785, 0.755));
    }
    path.closeSubpath();
    return path;
}

} // namespace

VehicleIconItem::VehicleIconItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setMipmap(true);
    setOpaquePainting(false);
}

void VehicleIconItem::paint(QPainter *painter)
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 1.0 || h <= 1.0) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setPen(Qt::NoPen);

    const QPainterPath body = bodyPath(w, h);

    painter->save();
    painter->translate(w * 0.045, h * 0.035);
    painter->setBrush(QColor(0, 0, 0, 76));
    painter->drawPath(body);
    painter->restore();

    painter->setBrush(QColor(30, 30, 28, 220));
    painter->drawPath(rounded(rect(w, h, 0.095, 0.255, 0.125, 0.225), w * 0.028));
    painter->drawPath(rounded(rect(w, h, 0.780, 0.255, 0.125, 0.225), w * 0.028));
    painter->drawPath(rounded(rect(w, h, 0.080, 0.625, 0.125, 0.240), w * 0.028));
    painter->drawPath(rounded(rect(w, h, 0.795, 0.625, 0.125, 0.240), w * 0.028));

    painter->setBrush(QColor(218, 216, 202, 255));
    painter->drawEllipse(rect(w, h, 0.075, 0.375, 0.130, 0.085));
    painter->drawEllipse(rect(w, h, 0.795, 0.375, 0.130, 0.085));

    QLinearGradient bodyGradient(0, 0, 0, h);
    bodyGradient.setColorAt(0.00, QColor(244, 242, 230));
    bodyGradient.setColorAt(0.30, QColor(232, 229, 212));
    bodyGradient.setColorAt(0.58, QColor(251, 250, 240));
    bodyGradient.setColorAt(0.86, QColor(235, 231, 214));
    bodyGradient.setColorAt(1.00, QColor(204, 199, 184));
    painter->setBrush(bodyGradient);
    painter->drawPath(body);

    painter->setPen(QPen(QColor(112, 113, 105, 230), qMax<qreal>(1.0, w * 0.018)));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(body);

    painter->setPen(QPen(QColor(255, 255, 250, 150), qMax<qreal>(1.0, w * 0.018)));
    painter->drawLine(pt(w, h, 0.335, 0.075), pt(w, h, 0.665, 0.075));

    QLinearGradient glassGradient(0, h * 0.12, 0, h * 0.55);
    glassGradient.setColorAt(0.00, QColor(92, 91, 78));
    glassGradient.setColorAt(0.28, QColor(63, 64, 57));
    glassGradient.setColorAt(0.72, QColor(35, 38, 35));
    glassGradient.setColorAt(1.00, QColor(77, 77, 67));
    painter->setPen(QPen(QColor(235, 235, 220, 190), qMax<qreal>(1.0, w * 0.014)));
    painter->setBrush(glassGradient);
    painter->drawPath(rearWindowPath(w, h));

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 250, 90));
    QPainterPath glassHighlight;
    glassHighlight.moveTo(pt(w, h, 0.370, 0.180));
    glassHighlight.cubicTo(pt(w, h, 0.430, 0.150), pt(w, h, 0.570, 0.150), pt(w, h, 0.630, 0.180));
    glassHighlight.lineTo(pt(w, h, 0.605, 0.225));
    glassHighlight.cubicTo(pt(w, h, 0.545, 0.200), pt(w, h, 0.455, 0.200), pt(w, h, 0.395, 0.225));
    glassHighlight.closeSubpath();
    painter->drawPath(glassHighlight);

    painter->setBrush(QColor(235, 232, 215, 180));
    painter->setPen(QPen(QColor(154, 151, 137, 180), qMax<qreal>(0.8, w * 0.010)));
    painter->drawPath(rearHatchPath(w, h));

    painter->setPen(QPen(QColor(142, 140, 128, 180), qMax<qreal>(0.9, w * 0.010)));
    painter->drawLine(pt(w, h, 0.300, 0.590), pt(w, h, 0.700, 0.590));
    painter->drawLine(pt(w, h, 0.305, 0.825), pt(w, h, 0.695, 0.825));

    painter->setPen(QPen(QColor(255, 255, 248, 170), qMax<qreal>(1.0, w * 0.015)));
    QPainterPath leftShoulder;
    leftShoulder.moveTo(pt(w, h, 0.245, 0.300));
    leftShoulder.cubicTo(pt(w, h, 0.265, 0.470), pt(w, h, 0.270, 0.720), pt(w, h, 0.335, 0.925));
    painter->drawPath(leftShoulder);
    QPainterPath rightShoulder;
    rightShoulder.moveTo(pt(w, h, 0.755, 0.300));
    rightShoulder.cubicTo(pt(w, h, 0.735, 0.470), pt(w, h, 0.730, 0.720), pt(w, h, 0.665, 0.925));
    painter->drawPath(rightShoulder);

    painter->setPen(Qt::NoPen);
    QLinearGradient lampGradient(0, h * 0.69, 0, h * 0.77);
    lampGradient.setColorAt(0.00, QColor(205, 38, 44));
    lampGradient.setColorAt(0.52, QColor(150, 12, 25));
    lampGradient.setColorAt(1.00, QColor(230, 82, 74));
    painter->setBrush(lampGradient);
    painter->drawPath(tailLightPath(w, h, true));
    painter->drawPath(tailLightPath(w, h, false));

    painter->setPen(QPen(QColor(95, 92, 82, 160), qMax<qreal>(0.8, w * 0.010)));
    painter->drawLine(pt(w, h, 0.235, 0.790), pt(w, h, 0.765, 0.790));

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(219, 214, 197, 230));
    painter->drawPath(rounded(rect(w, h, 0.410, 0.800, 0.180, 0.050), w * 0.010));

    painter->setBrush(QColor(130, 126, 113, 160));
    painter->drawPath(rounded(rect(w, h, 0.260, 0.920, 0.150, 0.025), w * 0.010));
    painter->drawPath(rounded(rect(w, h, 0.590, 0.920, 0.150, 0.025), w * 0.010));
}
