# 开发环境

## 支持版本

- Windows 10 / Windows 11
- Qt 6.6 或更高的兼容 Qt 6 版本
- CMake 3.21 或更高版本
- C++20 编译器
- Qt 模块：Core、Gui、Qml、Quick、QuickControls2、Test

后续开始通信功能时将增加 Qt WebSockets 模块。

## 配置与构建

推荐在 Qt Creator 中选择有效的 Desktop Qt Kit，然后配置 CMake 项目。

也可以在 PowerShell 中通过本机环境提供 Qt 路径：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=<本机Qt Kit路径>
cmake --build build
ctest --test-dir build --output-on-failure
```

不要把具体电脑上的 Qt 安装目录、编译器目录或构建目录提交到共享文件。

## 运行

```powershell
.\build\vehicle_ground_station.exe
```

应用支持 `--smoke-test` 参数。该模式会加载 QML 主界面并自动退出，适合验证资源和 QML 是否能够正常初始化。

## 当前已验证环境

首个基础切片使用 Qt 6.9.0 MinGW 64-bit Kit 完成了本地验证。不同电脑可使用兼容的 Qt 6 Kit，无需保持相同安装路径。
