# Qt OpenGL + QML 仪表盘 Demo

这是一个基于 Qt 5 的车机仪表盘示例项目：

- QML 绘制仪表盘 UI、速度数字、电量、状态信息。
- OpenGL 绘制中间道路、车道线、直道/弯道动画。
- C++ 原生 `QQuickPaintedItem + QPainter` 绘制车辆图标。
- 速度变化会同步影响码表数字、道路流动速度和车辆行驶动画。
- 动画会在直线加速、弯道减速、出弯加速之间循环，并随机插入直线减速再加速场景。

## 项目结构

```text
.
├── CMakeLists.txt
├── dashboard-demo.pro
├── qml.qrc
├── qml/
│   └── main.qml
└── src/
    ├── main.cpp
    ├── DashboardGlItem.h
    ├── DashboardGlItem.cpp
    ├── VehicleIconItem.h
    └── VehicleIconItem.cpp
```

## 运行环境

当前机器已验证可用环境：

```text
macOS
Homebrew
Qt 5.15.19 from Homebrew qt@5
qmake 3.1
CMake 4.3.3
Apple clang
```

项目代码按 Qt 5.14 兼容 API 编写。当前机器实际安装的是 Homebrew `qt@5`，版本为 Qt 5.15.19，可以正常构建运行。

## 需要安装的组件

### macOS

安装 Xcode Command Line Tools：

```bash
xcode-select --install
```

安装 Homebrew 后，安装 Qt5 和 CMake：

```bash
brew install qt@5 cmake
```

确认 Qt5 可用：

```bash
/usr/local/opt/qt@5/bin/qmake -v
```

预期能看到类似：

```text
QMake version 3.1
Using Qt version 5.15.19
```

### Linux / 嵌入式 Wayland

如果在 Ubuntu/Debian 类环境中构建，可参考：

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  qtbase5-dev \
  qtdeclarative5-dev \
  qml-module-qtquick2 \
  qml-module-qtquick-window2 \
  libgl1-mesa-dev \
  libegl1-mesa-dev \
  wayland-protocols
```

车机板端需要 Qt 是带 OpenGL ES、EGL、Wayland 支持的构建。

## 构建方式

### 方式一：qmake

推荐当前机器使用这个方式：

```bash
cd /Users/hunterdick/automation-project
mkdir -p build-qmake
cd build-qmake
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
```

构建完成后会生成：

```text
build-qmake/dashboard-demo.app
```

### 方式二：CMake

如果你的 Qt5 已经配置到 `CMAKE_PREFIX_PATH`，可以使用：

```bash
cd /Users/hunterdick/automation-project
cmake -S . -B build
cmake --build build -j4
```

如果 CMake 找不到 Qt5，可以显式指定：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5
cmake --build build -j4
```

## 启动方式

### macOS 启动

```bash
cd /Users/hunterdick/automation-project
open build-qmake/dashboard-demo.app
```

也可以直接运行二进制：

```bash
./build-qmake/dashboard-demo.app/Contents/MacOS/dashboard-demo
```

### Wayland 启动

在嵌入式 Linux/Wayland 环境中：

```bash
export QT_QPA_PLATFORM=wayland
./dashboard-demo
```

如果使用 Weston：

```bash
weston &
export QT_QPA_PLATFORM=wayland
./dashboard-demo
```

### X11 启动

如果是在 Linux 桌面 X11 环境：

```bash
export QT_QPA_PLATFORM=xcb
./dashboard-demo
```

## 当前动画逻辑

`qml/main.qml` 中实现了循环场景状态机：

```text
直线加速
  -> 入弯减速，码表数字下降
  -> 弯中低速行驶
  -> 出弯回正，车辆加速，码表数字上升
  -> 回到直线
```

直线阶段会随机插入：

```text
直线减速
  -> 直线恢复加速
  -> 再进入弯道
```

主要调参入口：

```text
qml/main.qml
  beginScene()
    targetSpeed
    targetCurve
    sceneDuration
```

## 关键文件说明

### `qml/main.qml`

负责：

- 仪表盘整体 UI。
- 速度、电量、右侧状态信息。
- 场景状态机。
- 车辆位置、缩放、转向。

### `src/DashboardGlItem.cpp`

负责：

- OpenGL FBO 渲染。
- 透视道路。
- 弯道形变。
- 车道虚线滚动。

### `src/VehicleIconItem.cpp`

负责：

- C++ 原生车辆图标绘制。
- 使用 `QPainterPath`、`QLinearGradient`、抗锯齿绘制车身、后窗、尾灯、轮胎和阴影。

## 常见问题

### CMake 找不到 Qt5

使用 qmake 构建，或指定 Qt5 路径：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5
```

### macOS 编译时报 `type_traits file not found`

当前 `.pro` 和 `CMakeLists.txt` 已显式加入 macOS SDK 的 libc++ 头文件路径。正常情况下直接重新执行：

```bash
cd build-qmake
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
```

### QML 中 OpenGL 图层不显示

`src/main.cpp` 中已强制 Qt Quick 使用 OpenGL 场景图：

```cpp
QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
```

这是为了避免 Qt 5.15 在 macOS 上走 Metal/RHI 后影响 `QQuickFramebufferObject`。

## 清理构建产物

```bash
cd /Users/hunterdick/automation-project
rm -rf build build-qmake
```

然后重新构建：

```bash
mkdir -p build-qmake
cd build-qmake
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
open dashboard-demo.app
```
