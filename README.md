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

项目代码按 Qt 5.14 兼容接口编写，主要面向 Linux/Wayland 或 Linux/X11 环境运行。macOS 也可以作为开发和预览环境。

推荐目标环境：

```text
Linux
Qt 5.14+
Qt Quick / QML
OpenGL or OpenGL ES
Wayland or X11
qmake or CMake
```

当前开发机已验证：

```text
macOS
Homebrew
Qt 5.15.19 from Homebrew qt@5
qmake 3.1
CMake 4.3.3
Apple clang
```

## Linux 使用手册

### 安装依赖

Ubuntu/Debian 类环境：

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  qtbase5-dev \
  qtbase5-dev-tools \
  qtdeclarative5-dev \
  qtdeclarative5-dev-tools \
  qml-module-qtquick2 \
  qml-module-qtquick-window2 \
  qtwayland5 \
  libgl1-mesa-dev \
  libegl1-mesa-dev \
  mesa-utils \
  wayland-protocols
```

如果需要在 X11 桌面运行，通常还需要 Qt 的 xcb 平台插件依赖。桌面版 Ubuntu/Debian 一般会自动带上，如果遇到 `Could not load the Qt platform plugin "xcb"`，可补装：

```bash
sudo apt install -y libxcb-xinerama0 libxkbcommon-x11-0
```

确认 Qt5 和 OpenGL 可用：

```bash
qmake -v
cmake --version
glxinfo -B
```

如果系统里的 Qt 安装的是 `qmake-qt5`，后续命令中的 `qmake` 替换为 `qmake-qt5`。

车机板端需要确认 Qt 构建带有 OpenGL ES、EGL、Wayland 支持。常见检查方式：

```bash
qtpaths --plugin-dir
find "$(qtpaths --plugin-dir)" -maxdepth 2 -type f | grep -E 'wayland|egl|xcb'
```

### Linux 构建

推荐优先使用 qmake，和 Qt 5 工程配置最直接：

```bash
cd /path/to/automation-project
rm -rf build-qmake
mkdir -p build-qmake
cd build-qmake
qmake ../dashboard-demo.pro
make -j"$(nproc)"
```

如果你的系统使用 `qmake-qt5`：

```bash
qmake-qt5 ../dashboard-demo.pro
make -j"$(nproc)"
```

构建完成后生成：

```text
build-qmake/dashboard-demo
```

CMake 构建方式：

```bash
cd /path/to/automation-project
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

如果 CMake 找不到 Qt5，指定 Qt5 安装路径，例如：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5
cmake --build build -j"$(nproc)"
```

不同发行版的 Qt5 CMake 路径可能不同，可以用下面的命令查找：

```bash
find /usr -path '*cmake/Qt5/Qt5Config.cmake' 2>/dev/null
```

### Linux 启动

Wayland 环境：

```bash
cd /path/to/automation-project/build-qmake
export QT_QPA_PLATFORM=wayland
./dashboard-demo
```

Weston 测试环境：

```bash
weston &
export QT_QPA_PLATFORM=wayland
./dashboard-demo
```

X11 环境：

```bash
cd /path/to/automation-project/build-qmake
export QT_QPA_PLATFORM=xcb
./dashboard-demo
```

如果想打开 Qt 平台插件调试日志：

```bash
QT_DEBUG_PLUGINS=1 QT_QPA_PLATFORM=wayland ./dashboard-demo
```

### 嵌入式/车机板端部署

通常只需要拷贝构建出的可执行文件即可，因为 QML 和 GLB 模型都已通过 `qml.qrc` 打包进二进制：

```bash
scp build-qmake/dashboard-demo user@target:/opt/dashboard-demo/
```

在目标设备运行：

```bash
ssh user@target
cd /opt/dashboard-demo
export QT_QPA_PLATFORM=wayland
./dashboard-demo
```

如果目标板使用 EGLFS 而不是 Wayland：

```bash
export QT_QPA_PLATFORM=eglfs
./dashboard-demo
```

如果目标设备有多个显示输出，可能还需要根据平台设置厂商相关环境变量，例如 `QT_QPA_EGLFS_KMS_CONFIG`。这部分取决于板卡 BSP 和 Qt 构建方式。

## macOS 开发环境

macOS 主要用于本地开发和预览。

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

## macOS 构建方式

### qmake

当前 macOS 开发机推荐使用 qmake：

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

## macOS 启动方式

### macOS

```bash
cd /Users/hunterdick/automation-project
open build-qmake/dashboard-demo.app
```

也可以直接运行二进制：

```bash
./build-qmake/dashboard-demo.app/Contents/MacOS/dashboard-demo
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

### Linux CMake 找不到 Qt5

优先使用 qmake 构建，或查找并指定 Qt5 路径：

```bash
find /usr -path '*cmake/Qt5/Qt5Config.cmake' 2>/dev/null
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/qt5/cmake/prefix
```

Ubuntu/Debian 常见路径是：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5
```

### Linux 启动时报 Wayland 插件错误

确认安装了 `qtwayland5`：

```bash
sudo apt install -y qtwayland5
```

确认当前 session 是 Wayland：

```bash
echo "$XDG_SESSION_TYPE"
echo "$WAYLAND_DISPLAY"
```

如果没有 Wayland compositor，可以先用 X11：

```bash
QT_QPA_PLATFORM=xcb ./dashboard-demo
```

### Linux 启动时报 xcb 插件错误

补装常见 xcb 依赖：

```bash
sudo apt install -y libxcb-xinerama0 libxkbcommon-x11-0
```

查看插件加载细节：

```bash
QT_DEBUG_PLUGINS=1 QT_QPA_PLATFORM=xcb ./dashboard-demo
```

### Linux 上 OpenGL 图层不显示

确认 GPU/mesa 信息：

```bash
glxinfo -B
```

如果是 Wayland/EGL 环境，确认 Qt 平台插件和 EGL 库存在：

```bash
qtpaths --plugin-dir
ldconfig -p | grep -E 'libEGL|libGLES|libGL'
```

本项目在 `src/main.cpp` 中强制 Qt Quick 使用 OpenGL scene graph：

```cpp
QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
```

如果目标板只支持 OpenGL ES，可能需要根据目标 Qt 构建调整该属性或 Qt 平台插件配置。

### macOS 编译时报 `type_traits file not found`

当前 `.pro` 和 `CMakeLists.txt` 已显式加入 macOS SDK 的 libc++ 头文件路径。重新执行：

```bash
cd /Users/hunterdick/automation-project
mkdir -p build-qmake
cd build-qmake
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
```

### macOS 中 OpenGL 图层不显示

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
qmake ../dashboard-demo.pro
make -j"$(nproc)"
./dashboard-demo
```

macOS 上使用：

```bash
/usr/local/opt/qt@5/bin/qmake ../dashboard-demo.pro
make -j4
open dashboard-demo.app
```
