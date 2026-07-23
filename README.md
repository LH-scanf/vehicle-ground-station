# Vehicle Ground Station

面向单台 ROS2 无人车的 Windows 地面站。项目使用 C++20、Qt 6、Qt Quick/QML 和 CMake；Windows 应用不直接依赖 ROS2。

当前 V0.1 基础切片包含主窗口、状态栏、页面导航、C++ 车辆状态对象、模拟车辆数据和本地连接配置。WebSocket、真实地图与车辆控制尚未实现。

## 开发环境

- Windows 10 或 Windows 11
- Qt 6.6 或更高的兼容 Qt 6 版本
- CMake 3.21 或更高版本
- 与 Qt Kit 匹配的 C++ 编译器

Qt 路径应由 Qt Creator Kit、`CMAKE_PREFIX_PATH` 或本机 CMake Preset 提供，不应写入仓库。

## 构建

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=<本机Qt Kit路径>
cmake --build build
ctest --test-dir build --output-on-failure
```

启动应用：

```powershell
.\build\vehicle_ground_station.exe
```

执行无界面启动检查：

```powershell
.\build\vehicle_ground_station.exe --smoke-test
```

详细环境说明见 `docs/development_setup.md`。
