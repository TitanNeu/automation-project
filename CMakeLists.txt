cmake_minimum_required(VERSION 3.14)

project(QtDashboardOpenGL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.14 REQUIRED COMPONENTS Core Gui Qml Quick)

if(APPLE)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

add_executable(dashboard-demo
    src/main.cpp
    src/DashboardGlItem.h
    src/DashboardGlItem.cpp
    src/VehicleIconItem.h
    src/VehicleIconItem.cpp
    qml.qrc
)

target_link_libraries(dashboard-demo
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Qml
        Qt5::Quick
)

if(APPLE AND MACOS_SDK_PATH)
    target_include_directories(dashboard-demo SYSTEM PRIVATE
        "${MACOS_SDK_PATH}/usr/include/c++/v1"
    )
endif()
