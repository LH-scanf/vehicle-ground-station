# 软件架构

## 1. 架构目标

Windows 地面站采用分层结构，保持界面、状态、业务、通信与车辆端解耦。Windows 应用只使用 WebSocket 与车辆网关通信，不直接依赖 ROS2。

```text
QML 界面
   ↓ Qt 属性、信号和槽
C++ 应用与状态层
   ↓ 明确的业务接口
C++ 通信与协议层
   ↓ WebSocket + JSON
Linux 车辆网关
   ↓
ROS2
```

## 2. 当前模块

### QML 界面层

`qml/` 只负责页面布局、视觉展示和基本交互。界面读取 C++ 对象暴露的属性，并调用受控的保存操作。JSON 解析、文件读写和权威车辆状态不放在 QML 中。

### VehicleState

`src/vehicle/VehicleState` 是当前车辆状态的 C++ 表示，通过 `Q_PROPERTY`、通知信号和槽暴露给 QML。其数据只由通过协议校验的`vehicle_status`更新；开发联调模拟器位于ROS仓库并发布正常ROS Topic，不在Windows应用内伪造车辆状态。

### ConfigManager

`src/config/ConfigManager` 负责配置加载、覆盖、校验和保存：

1. 从应用资源读取共享的 `config/default_config.json`。
2. 若可执行文件旁的 `config/local_config.json` 存在，则按字段覆盖共享默认值。
3. 本地文件缺失时正常运行；文件损坏或字段无效时保留可用默认值并报告错误。
4. 设置页面保存时只原子写入本地配置，不改写共享默认配置。

本地配置属于机器或操作员环境，不进入 Git。

### LogManager

`src/log/LogManager` 同时承担结构化日志模型和日志事件入口。所有日志使用统一字段，按 `system`、`configuration`、`communication`、`operation`、`command`、`state`、`alarm` 和 `error` 分类。

日志的 `display` 字段只控制展示方式，不是登录或权限角色：

- `primary`：默认显示在“运行记录 / 诊断”页面。
- `diagnostic`：用户开启“显示技术详情”后显示。
- `file_only`：只写入磁盘文件。

`LogFilterModel` 负责前端等级、类型、技术详情和关键字筛选。清空当前显示只清理内存模型，并额外写入一条仅文件可见的操作记录，不删除磁盘日志。

`LogWriter` 位于专用线程，按本地日期将 UTF-8 JSON Lines 写入日志目录。每行都是一条完整 JSON，文件名为 `yyyy-MM-dd.jsonl`。启动时根据配置清理过期日志文件。

普通日志不保存高频遥测、完整地图、完整路径或敏感信息。需要持续记录原始车辆数据时，应建立独立的数据记录与回放模块。

### ProtocolValidator

`src/communication/ProtocolValidator`实现V1 JSON公共消息结构和当前通信切片所需的`gateway_ready`、`pong`、`vehicle_status`校验。协议校验不依赖QML，也不直接修改车辆状态；只有完整验证通过的遥测才能进入`VehicleState`。

### WebSocketClient

`src/communication/WebSocketClient`在Qt事件循环中异步拥有`QWebSocket`，负责连接、手动断开、3秒自动重连、1秒JSON心跳、3秒心跳超时、网关就绪门控和遥测陈旧检测。该模块不阻塞界面线程。

当前通信切片处理：

- `event/gateway_ready`
- `heartbeat/pong`
- `telemetry/vehicle_status`
- `command/set_mode`发送
- `ack/set_mode`接收及发送、接受、完成、失败、超时和结果未知状态

`WebSocketClient`当前只允许一个模式切换命令处于等待状态。它生成唯一`request_id`，在断线时把未完成命令标记为“结果未知”，并且不会自动重发。收到`completed`也不会直接修改`VehicleState.mode`；实际模式仍只能由经过验证的`vehicle_status`更新。尚未实现的急停、人工控制、导航、任务取消、`alarm`等消息只记录诊断信息，不产生车辆控制副作用。二进制WebSocket消息会按V1规则拒绝。

## 3. 启动流程

```text
QGuiApplication
   → ConfigManager 加载默认值和本地覆盖
   → LogManager 根据配置初始化日志目录和写入线程
   → 创建 VehicleState（初始离线）
   → 创建 WebSocketClient 并绑定配置、日志和车辆状态
   → 将配置、日志模型和车辆状态暴露给 QML
   → 加载 Main.qml
   → 若本机配置启用auto_connect，则连接车辆网关
```

默认配置无法读取时应用仍可启动，但会记录警告并使用编译时安全默认值。本地配置错误不会导致程序退出。

## 4. 目录职责

- `src/config/`：配置解析、校验与持久化。
- `src/log/`：结构化日志、界面日志模型、筛选和后台文件写入。
- `src/communication/`：WebSocket连接、心跳、协议解析与校验。
- `src/vehicle/`：权威车辆状态。
- `qml/`：页面、组件和视觉交互。
- `config/`：提交到 Git 的共享默认配置。
- `tests/`：可独立执行的 C++ 行为测试。
- `docs/tasks/`：每个开发切片的范围和验收条件。

后续地图、任务、控制和告警模块按 `AGENTS.md` 规定加入各自的C++目录，不提前创建空类。

## 5. 线程与安全边界

当前配置读写发生在启动或用户主动保存时，数据量很小。日志磁盘写入和过期文件清理由日志工作线程执行。后续网络接收和地图处理同样不得阻塞界面线程。

QML 展示的模式和急停状态最终必须来自车辆实际遥测。命令发送、车辆应答、执行成功和超时是不同状态，不得由界面本地状态代替。
