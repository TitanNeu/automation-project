QT += quick qml gui
CONFIG += c++11

TARGET = dashboard-demo
TEMPLATE = app

macx {
    # Apple CLT 26 keeps libc++ headers under the selected SDK, while Qt 5's
    # older macx-clang mkspec still expects them under CommandLineTools/usr.
    SDK_CPP_HEADERS = $$system(xcrun --show-sdk-path)/usr/include/c++/v1
    QMAKE_CXXFLAGS += -isystem $$SDK_CPP_HEADERS
}

SOURCES += \
    src/main.cpp \
    src/DashboardGlItem.cpp

HEADERS += \
    src/DashboardGlItem.h

RESOURCES += qml.qrc
