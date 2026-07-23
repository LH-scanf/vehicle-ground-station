# 任务 001：基础可运行界面

## 目标

建立一个可配置、可编译、可启动的 Qt 6 工程，贯通 C++ 车辆状态对象与 QML 界面。

## 范围

- 主窗口、顶部状态栏和侧边导航
- Dashboard、Map、Diagnostics、Settings 页面
- C++ `VehicleState` 通过 Qt 属性暴露给 QML
- 定时更新的模拟车辆数据
- C++ 属性单元测试和应用启动检查

## 不在本任务范围内

- WebSocket 和 JSON 协议解析
- ROS2 集成
- 真实地图、任务与人工控制
- 配置持久化、SQLite 和日志文件

## 验收条件

1. CMake 配置和构建成功。
2. Qt Test 测试通过。
3. 应用能够加载全部 QML 页面。
4. Dashboard 显示来自 C++ 的模拟车辆状态。
5. 仓库不包含机器专用路径或构建输出。
