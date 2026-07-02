#pragma once

#include <QQuickFramebufferObject>

class DashboardGlItem : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(qreal distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(qreal curve READ curve WRITE setCurve NOTIFY curveChanged)

public:
    explicit DashboardGlItem(QQuickItem *parent = nullptr);

    Renderer *createRenderer() const override;

    qreal speed() const;
    void setSpeed(qreal speed);

    qreal distance() const;
    void setDistance(qreal distance);

    qreal curve() const;
    void setCurve(qreal curve);

signals:
    void speedChanged();
    void distanceChanged();
    void curveChanged();

private:
    qreal m_speed = 0.0;
    qreal m_distance = 0.0;
    qreal m_curve = 0.0;
};
