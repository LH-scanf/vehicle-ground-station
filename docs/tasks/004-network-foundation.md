# 004 — WebSocket连接、心跳与遥测

## 目标

实现Windows Qt地面站与`vehicle_gateway`的第一轮只读通信闭环，为后续命令、导航和安全控制提供经过验证的连接基础。

## 本次范围

- 使用Qt WebSockets建立和断开连接。
- 从本机配置生成`ws://<vehicle_ip>:<vehicle_port>`。
- 支持启动自动连接和断线3秒后自动重连。
- 每1秒发送V1 `heartbeat/ping`，验证对应`pong`并计算往返时间。
- 3秒未收到可匹配`pong`时关闭连接并进入超时状态。
- 验证`event/gateway_ready`，确认协议版本和`cancel_task_and_stop`策略。
- 验证`telemetry/vehicle_status`并更新C++ `VehicleState`。
- 遥测超过3秒未更新时标记车辆离线，但保持WebSocket会话以等待恢复。
- 记录连接、断开、重连、心跳超时和协议校验失败日志。
- 在设置页提供手动连接、断开和连接诊断信息。

## 不在本次范围

- 发送任何`command`。
- 处理命令`ack`生命周期。
- 模式切换、人工控制、急停、导航和任务取消。
- 地图与路径消息。
- TLS、认证和公网连接。

## 关键规则

1. QML不解析JSON，也不直接拥有`QWebSocket`。
2. 只有完整通过协议验证的遥测才能更新`VehicleState`。
3. `gateway_ready`之后仍需收到新鲜`vehicle_status`才将车辆显示为在线。
4. 自动重连只恢复链路，不恢复任何旧状态或控制输入。
5. 普通日志不保存10Hz完整遥测和完整原始JSON。
6. V1只接受UTF-8 JSON文本消息，单条上限256 KiB；二进制消息拒绝。
7. 车辆网关使用局域网直连，WebSocket客户端不使用系统、PAC或VPN代理。

## 验收

- 项目使用Qt 6.6以上兼容Kit编译成功。
- 公共协议、网关就绪、心跳和遥测校验具有Qt单元测试。
- 使用本地`QWebSocketServer`完成连接、就绪、ping/pong和遥测集成测试。
- 原有配置、日志和车辆状态测试继续通过。
- QML无界面启动检查通过。
- 未连接网关时程序仍可正常启动，车辆初始显示离线。

## 联调步骤

1. Ubuntu启动ROS2和`vehicle_gateway`，监听局域网地址的8765端口。
2. Windows设置页填写Ubuntu局域网IP、端口和相同的`vehicle_id`并保存。
3. 点击“连接网关”，确认依次出现“等待网关就绪”“等待车辆遥测”“在线”。
4. 确认心跳往返时间持续更新，状态总览显示ROS2遥测。
5. 停止网关，确认地面站在3秒内显示离线并按配置尝试重连。
