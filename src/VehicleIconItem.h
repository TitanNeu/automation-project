#pragma once

#include <QQuickPaintedItem>

class VehicleIconItem : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit VehicleIconItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
};
