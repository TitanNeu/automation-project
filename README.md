# Qt OpenGL + QML 仪表盘 Demo

这是一个基于 Qt 5.14 兼容 API 的车机仪表盘演示项目。当前版本使用 QML 绘制仪表盘 UI，使用 OpenGL 绘制中间道路、道路标线和外部导入的 GLB 车辆模型。

## 功能概览

- QML 绘制速度表、电量、左右状态信息和底部状态栏。
- OpenGL 绘制透视道路、直道/弯道、车道虚线、横向路纹和车辆模型。
- 车辆固定在道路近端中央，行驶感由道路中线和横向路纹从远端向近端流动表示。
- 动画在直线加速、弯道减速、弯中低速、出弯加速之间循环。
- 随机插入直线减速再加速场景。
- 车辆模型来自外部 GLB 文件，已打包进 Qt resource。

## 项目结构

```text
.
├── CMakeLists.txt
├── dashboard-demo.pro
├── qml.qrc
├── README.md
├── assets/
│   └── models/
│       ├── README.md
│       ├── quaternius-white-sedan.glb
│       └── quaternius-white-sedan-preview.png
├── qml/
│   └── main.qml
└── src/
    ├── main.cpp
    ├── DashboardGlItem.h
    └── DashboardGlItem.cpp
```

## 运行环境

当前机器已验证：

```text
macOS
Homebrew
Qt 5.15.19 from Homebrew qt@5
qmake 3.1
CMake 4.3.3
Apple clang
```

项目代码按 Qt 5.14 兼容接口编写。当前机器实际安装的是 Homebrew `qt@5`，版本为 Qt 5.15.19，可以正常构建运行。

## 需要安装的组件

### macOS

安装 Xcode Command Line Tools：

```bash
xcode-select --install
```

安装 Homebrew 后安装 Qt5 和 CMake：

```bash
brew install qt@5 cmake
```

确认 Qt5 可用：

```bash
/usr/local/opt/qt@5/bin/qmake -v
```

预期输出类似：

```text
QMake version 3.1
Using Qt version 5.15.19
```

### Linux / 嵌入式 Wayland

Ubuntu/Debian 类环境可参考：

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

车机板端需要 Qt 构建带 OpenGL ES、EGL 和 Wayland 支持。

## 构建方式

### qmake

当前机器推荐使用 qmake：

```bash
cd /Users/hunterdick/automation-project
mkdir -p build-qmake
cd build-qmake
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
```

构建完成后生成：

```text
build-qmake/dashboard-demo.app
```

### CMake

如果 Qt5 已配置到 `CMAKE_PREFIX_PATH`：

```bash
cd /Users/hunterdick/automation-project
cmake -S . -B build
cmake --build build -j4
```

如果 CMake 找不到 Qt5，显式指定路径：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5
cmake --build build -j4
```

## 启动方式

### macOS

```bash
cd /Users/hunterdick/automation-project
open build-qmake/dashboard-demo.app
```

也可以直接运行二进制：

```bash
./build-qmake/dashboard-demo.app/Contents/MacOS/dashboard-demo
```

### Wayland

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

### X11

Linux 桌面 X11 环境：

```bash
export QT_QPA_PLATFORM=xcb
./dashboard-demo
```

## 当前动画逻辑

`qml/main.qml` 负责场景状态机：

```text
直线加速
  -> 入弯减速，码表数字下降
  -> 弯中低速行驶
  -> 出弯回正，码表数字上升
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

## 车辆模型

当前车辆使用外部低面数 GLB 模型：

```text
assets/models/quaternius-white-sedan.glb
```

模型来源：

- Model page: https://poly.pizza/m/unqqkULtRU
- Creator: Quaternius
- License: Public Domain (CC0)
- Approximate triangle count: 3,124

`qml.qrc` 已将 GLB 打包进 Qt resource：

```xml
<file>assets/models/quaternius-white-sedan.glb</file>
```

OpenGL 渲染层会在运行时从 `qrc:/assets/models/quaternius-white-sedan.glb` 对应的资源路径读取模型数据。

## 关键文件说明

### `qml/main.qml`

负责：

- 仪表盘整体 UI。
- 速度、电量、右侧状态信息。
- 直线/弯道/减速/加速场景状态机。
- 将 `speed`、`distance`、`curve` 传给 OpenGL 道路视图。

### `src/DashboardGlItem.cpp`

负责：

- `QQuickFramebufferObject` OpenGL FBO 渲染。
- 透视道路、弯道形变、车道边缘、中线虚线、横向路纹。
- 读取 GLB 文件中的 mesh、accessor、bufferView、material 数据。
- 将车辆模型投影到仪表盘道路中央。
- 固定车辆屏幕位置，通过道路纹理流动表达行驶速度。

### `src/main.cpp`

负责：

- 强制 Qt Quick 使用 OpenGL scene graph。
- 注册 `DashboardRoadView` QML 类型。
- 加载 `qrc:/qml/main.qml`。

## 常见问题

### CMake 找不到 Qt5

使用 qmake 构建，或指定 Qt5 路径：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5
```

### macOS 编译时报 `type_traits file not found`

当前 `.pro` 和 `CMakeLists.txt` 已显式加入 macOS SDK 的 libc++ 头文件路径。重新执行：

```bash
cd /Users/hunterdick/automation-project
mkdir -p build-qmake
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

### 车辆看起来不是完整真实 3D 相机效果

当前实现为了兼容现有 Qt 5/OpenGL 2D 道路层，使用轻量 GLB 解析和手动斜俯视投影，而不是完整 3D camera/projection matrix。这样更容易和仪表盘 HMI 合成，但车辆立体感主要依赖投影参数和材质明暗。

## 清理构建产物

```bash
cd /Users/hunterdick/automation-project
rm -rf build build-qmake
```

清理后重新构建：

```bash
mkdir -p build-qmake
cd build-qmake
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
open dashboard-demo.app
```
