#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>

#include "DashboardGlItem.h"
#include "VehicleIconItem.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QGuiApplication app(argc, argv);

    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    qmlRegisterType<DashboardGlItem>("Dashboard", 1, 0, "DashboardRoadView");
    qmlRegisterType<VehicleIconItem>("Dashboard", 1, 0, "VehicleIcon");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
