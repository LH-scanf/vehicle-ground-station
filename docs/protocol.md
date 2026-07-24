# Vehicle Ground Station WebSocket Protocol

- **文档状态：** Draft，供评审，不表示已经完成实现
- **协议版本：** V1
- **适用阶段：** 地面站 V0.1
- **最后更新：** 2026-07-23

---

## 1. 目的与范围

本文档定义 Windows Qt 地面站与 Linux `vehicle_gateway` 之间的 WebSocket JSON 协议。

```text
Windows Qt 地面站（WebSocket 客户端）
                 ↕
Linux vehicle_gateway（WebSocket 服务端）
                 ↕
                ROS2
```

V1 仅定义以下消息类型：

| `type` | 用途 |
|---|---|
| `heartbeat` | 心跳请求和应答 |
| `telemetry` | 车辆遥测 |
| `command` | 地面站命令 |
| `ack` | 命令接收或执行结果 |
| `event` | 模式、任务等状态变化 |
| `alarm` | 车辆告警 |
| `error` | 协议级错误 |

V1 暂不定义地图和路径消息，也不支持多车辆管理、登录鉴权、互联网控制或完整遥测录制。

Windows 应用不直接依赖 ROS2。ROS2 Topic、Service 和 Action 与本协议之间的转换由 `vehicle_gateway` 负责。

---

## 2. 传输规则

### 2.1 连接角色

- 地面站是 WebSocket 客户端。
- `vehicle_gateway` 是 WebSocket 服务端。
- 地址由地面站配置，V1 URL 格式为 `ws://<vehicle_ip>:<vehicle_port>`。
- V1 面向可信局域网，不允许直接暴露到互联网。
- TLS、身份认证和授权不属于 V1；未来需要远程网络时必须升级为 `wss://` 并增加认证设计。

### 2.2 帧格式

- 每个 WebSocket 文本帧包含一个完整 UTF-8 JSON 对象。
- 一个帧中不得拼接多个 JSON 对象。
- V1 不接收二进制帧。
- JSON 顶层必须是对象，不能是数组、字符串或数字。
- 单个UTF-8 JSON文本消息最大为256 KiB；超过上限时接收端关闭WebSocket会话。
- 超过实现上限时，接收端应记录错误并使用 WebSocket 关闭码 `1009` 关闭连接。

### 2.3 连接和重连

- 建议连接超时为 5 秒。
- 建议自动重连间隔为 3 秒。
- 连接断开后，地面站必须立即显示离线并停止发送普通控制命令。
- 重连成功后，旧连接上的 `seq` 不再连续，新会话从 1 开始。
- 断开时仍未收到最终结果的命令必须在地面站标记为“结果未知”，原 `request_id` 仅用于日志和历史记录。
- 自动重连只恢复通信链路，不恢复控制状态，也不自动重发任何旧命令。
- 重连后，地面站必须先收到新的 `gateway_ready` 和新鲜的 `vehicle_status`，才允许重新启用相应操作。
- 导航、模式切换、解除急停以及其他会改变车辆状态的旧命令，即使此前没有确认结果，也不得自动重发；需要操作员重新操作，并生成新的 `request_id`。
- 车辆端的通信失联保护必须独立于地面站重连逻辑。

---

## 3. 公共消息结构

除无法解析的错误文本外，所有 V1 消息使用以下结构：

```json
{
  "version": 1,
  "type": "telemetry",
  "name": "vehicle_status",
  "vehicle_id": "car_01",
  "seq": 1001,
  "timestamp": 1784651000123,
  "data": {}
}
```

### 3.1 公共字段

| 字段 | 类型 | 必需 | 说明 |
|---|---|---:|---|
| `version` | integer | 是 | 协议主版本，V1 固定为 `1` |
| `type` | string | 是 | 消息类型，只允许本文档定义的值 |
| `name` | string | 是 | 具体消息名称 |
| `vehicle_id` | string | 是 | 车辆编号，必须与当前连接配置一致 |
| `seq` | integer | 是 | 当前发送方、当前连接会话内的消息序号 |
| `timestamp` | integer | 是 | Unix Epoch 毫秒时间戳，使用 UTC 基准 |
| `request_id` | string | 条件必需 | 命令关联编号，规则见第7节 |
| `data` | object | 是 | 具体消息数据；无内容时使用空对象 `{}` |

### 3.2 字段约束

- `vehicle_id` 长度建议为 1～64 个字符。
- `seq` 使用无符号32位整数范围 `1..4294967295`，达到上限后从1重新开始。
- 每个发送方向独立维护 `seq`；客户端和服务端的序号不要求相同。
- `seq` 用于诊断丢包和乱序，不用于命令防重复。
- `timestamp` 可用于显示和事件排序，但安全超时必须使用接收端本机单调时钟判断。
- `request_id` 长度建议为 1～64 个字符，必须在地面站生成。
- 未知的可选字段应被忽略并保留向后兼容性。
- 必需字段缺失、字段类型错误或字段值越界时，当前消息无效。

### 3.3 命名规则

- `type`、`name`、枚举值和 JSON 字段使用小写 `snake_case`。
- 单位优先写入字段说明；容易混淆的新增字段应在名称中带单位后缀。
- 已发布字段不得静默改名或改变含义。

---

## 4. 消息验证与处理顺序

接收端按以下顺序验证：

1. 文本是否为合法 UTF-8。
2. 是否为合法 JSON 对象。
3. 公共字段是否存在且类型正确。
4. `version` 是否受支持。
5. `vehicle_id` 是否匹配。
6. `type` 和 `name` 是否已定义。
7. `data` 中的必需字段、类型、枚举和范围是否正确。
8. 命令是否满足当前模式、安全状态和任务状态限制。

单条坏消息通常只应被忽略、记录并返回 `error`，不应直接导致应用崩溃或连接断开。

以下情况可以断开连接：

- 消息超过大小上限。
- 连续发送大量非法消息。
- 协议主版本不兼容且双方无法继续通信。
- WebSocket 本身发生协议错误。

---

## 5. 心跳消息 `heartbeat`

### 5.1 行为

- WebSocket 建立后，由地面站每1秒发送一次 `ping`。
- 车辆网关应尽快返回对应 `pong`。
- 地面站超过3秒没有收到任何有效 `pong` 时，进入通信超时状态。
- 收到 `pong` 后，以地面站本机发送和接收时间计算往返延迟。
- 心跳用于确认网关业务循环可响应，不替代车辆端500毫秒控制命令超时保护。

### 5.2 `ping`

```json
{
  "version": 1,
  "type": "heartbeat",
  "name": "ping",
  "vehicle_id": "car_01",
  "seq": 101,
  "timestamp": 1784651000000,
  "data": {}
}
```

### 5.3 `pong`

```json
{
  "version": 1,
  "type": "heartbeat",
  "name": "pong",
  "vehicle_id": "car_01",
  "seq": 88,
  "timestamp": 1784651000030,
  "data": {
    "ping_seq": 101,
    "ping_timestamp": 1784651000000
  }
}
```

| `data`字段 | 类型 | 必需 | 说明 |
|---|---|---:|---|
| `ping_seq` | integer | 是 | 原 `ping` 的 `seq` |
| `ping_timestamp` | integer | 是 | 原 `ping` 的 `timestamp`，原样回传 |

无法匹配当前已发送 `ping` 的 `pong` 不得用于清除通信超时状态。

---

## 6. 遥测消息 `telemetry`

### 6.1 `vehicle_status`

车辆网关建议以不低于10Hz的频率发送车辆状态。

```json
{
  "version": 1,
  "type": "telemetry",
  "name": "vehicle_status",
  "vehicle_id": "car_01",
  "seq": 1001,
  "timestamp": 1784651000123,
  "data": {
    "x": 2.35,
    "y": 1.48,
    "yaw": 0.52,
    "latitude": 0.0,
    "longitude": 0.0,
    "speed": 0.8,
    "steer_norm": 0.1,
    "throttle_norm": 0.4,
    "mode": "auto",
    "battery_v": 24.6,
    "battery_pct": 82,
    "gps_fix": 4,
    "signal_rssi": -61,
    "rc_link": true,
    "mission_id": "mission_42",
    "mission_status": "running",
    "wp_idx": 1,
    "wp_total": 3,
    "wp_dist": 8.5,
    "estop_active": false,
    "comm_timeout": false,
    "error_code": 0
  }
}
```

### 6.2 必需状态字段

| 字段 | 类型 | 单位/范围 | 说明 |
|---|---|---|---|
| `x` | number | 米 | `map`坐标系中的车辆位置 |
| `y` | number | 米 | `map`坐标系中的车辆位置 |
| `yaw` | number | 弧度 | 从坐标系`+X`轴起逆时针为正 |
| `speed` | number | 米/秒 | 车辆当前有符号速度 |
| `mode` | string | 枚举 | `rc`、`auto`、`ground` |
| `battery_pct` | integer | 0～100 | 电池百分比 |
| `gps_fix` | integer | 见下表 | GPS定位状态 |
| `rc_link` | boolean | - | 遥控器链路是否有效 |
| `estop_active` | boolean | - | 急停是否激活 |
| `comm_timeout` | boolean | - | 车辆端是否检测到控制通信超时 |
| `error_code` | integer | `>=0` | `0`表示无车辆错误 |

协议只传输弧度制 `yaw`。界面需要显示角度或航向值时，由地面站将 `yaw` 转换为度并归一化到 `[0,360)`；协议不定义重复的角度制航向字段。

### 6.3 可选状态字段

| 字段 | 类型 | 单位/范围 | 说明 |
|---|---|---|---|
| `latitude` | number | 度 | WGS84纬度；不可用时可省略 |
| `longitude` | number | 度 | WGS84经度；不可用时可省略 |
| `steer_norm` | number | `[-1,1]` | 归一化转向，负值左转、正值右转，方向仍待实车确认 |
| `throttle_norm` | number | `[-1,1]` | 归一化驱动输入 |
| `battery_v` | number | 伏 | 电池电压 |
| `signal_rssi` | integer | dBm | 无线信号强度 |
| `mission_id` | string | - | 当前任务编号 |
| `mission_status` | string | 见第9节 | 当前任务状态 |
| `wp_idx` | integer | `>=0` | 当前航点序号 |
| `wp_total` | integer | `>=0` | 航点总数 |
| `wp_dist` | number | 米 | 距离下一航点的距离 |

### 6.4 GPS状态建议映射

| `gps_fix` | 含义 |
|---:|---|
| `0` | 无有效定位 |
| `1` | 单点定位 |
| `2` | 差分定位 |
| `4` | RTK Fixed |
| `5` | RTK Float |

该映射需要在接入实际GPS驱动前与车辆端确认。

### 6.5 状态处理规则

- 地面站必须以车辆实际遥测的 `mode` 和 `estop_active` 为准。
- 不得仅依据本地按钮状态认为模式已经切换或急停已经解除。
- 单条遥测字段无效时，不得用无效值覆盖最后一个有效状态。
- 状态长时间未更新时，地面站应标记数据陈旧并产生告警。
- 普通运行日志不应保存每一帧完整遥测。

---

## 7. 地面站命令 `command`

### 7.1 公共规则

- 除周期性的 `manual_control` 外，所有命令必须包含唯一 `request_id`。
- `request_id` 由地面站生成，不得仅使用可能重复的 `seq`。
- 有副作用的命令不得因为重连或超时而使用新 `request_id` 自动重试。
- 急停需要重发时，应使用原 `request_id`，直到收到应答或连接断开。
- 车辆网关不得重复执行同一个 `request_id`。
- 相同 `request_id`、命令名和数据的重发应返回已缓存的最新应答。
- 相同 `request_id`若携带不同命令名或数据，必须以`duplicate_request`拒绝。
- 活跃命令的 `request_id` 必须保留到最终结果；已完成命令建议至少缓存10分钟。

### 7.2 命令列表

| `name` | 用途 | 是否要求`request_id` | 是否要求`ack` |
|---|---|---:|---:|
| `set_mode` | 切换控制模式 | 是 | 是 |
| `set_estop` | 激活急停 | 是 | 是 |
| `clear_estop` | 解除急停 | 是 | 是 |
| `manual_control` | 周期性人工控制 | 否 | 否，状态拒绝时返回事件 |
| `stop_motion` | 主动停止普通运动 | 是 | 是 |
| `navigate_to_pose` | 开始单点导航 | 是 | 是 |
| `cancel_mission` | 取消任务 | 是 | 是 |

### 7.3 `set_mode`

```json
{
  "version": 1,
  "type": "command",
  "name": "set_mode",
  "vehicle_id": "car_01",
  "seq": 102,
  "request_id": "cmd_1002",
  "timestamp": 1784651000200,
  "data": {
    "mode": "ground"
  }
}
```

`mode`只允许 `rc`、`auto`、`ground`。最终成功必须以车辆实际模式已经改变为条件。

### 7.4 `set_estop`和`clear_estop`

```json
{
  "version": 1,
  "type": "command",
  "name": "set_estop",
  "vehicle_id": "car_01",
  "seq": 103,
  "request_id": "cmd_1003",
  "timestamp": 1784651000300,
  "data": {
    "reason": "operator_request"
  }
}
```

- `set_estop`优先级高于普通命令。
- `clear_estop`必须由操作员主动触发，禁止自动发送。
- 车辆网关可以根据车辆安全条件拒绝解除急停。
- 最终结果必须由 `ack` 和后续实际遥测共同确认。

### 7.5 `manual_control`

```json
{
  "version": 1,
  "type": "command",
  "name": "manual_control",
  "vehicle_id": "car_01",
  "seq": 104,
  "timestamp": 1784651000400,
  "data": {
    "speed": 0.5,
    "steer_norm": -0.2
  }
}
```

| 字段 | 类型 | 单位/范围 | 说明 |
|---|---|---|---|
| `speed` | number | 米/秒 | 期望有符号速度，必须受配置最大速度限制 |
| `steer_norm` | number | `[-1,1]` | 归一化转向值 |

规则：

- 只有车辆实际处于 `ground` 模式、连接有效且未急停时才允许发送。
- 地面站人工控制期间应周期性发送，建议10～20Hz。
- 车辆端超过500毫秒未收到有效人工控制命令时，必须独立将运动输出置零。
- 该高频命令不逐帧返回 `ack`，也不逐帧写普通日志。
- 字段超范围时返回协议`error`；当前状态不允许时发送`manual_control_rejected`事件。
- 无论是否返回错误或事件，不允许的控制消息都不得执行。

### 7.6 `stop_motion`

`data`使用空对象。该命令停止普通运动，但不等价于急停，也不能清除急停。

### 7.7 `navigate_to_pose`

```json
{
  "version": 1,
  "type": "command",
  "name": "navigate_to_pose",
  "vehicle_id": "car_01",
  "seq": 105,
  "request_id": "cmd_1005",
  "timestamp": 1784651000500,
  "data": {
    "mission_id": "mission_42",
    "frame_id": "map",
    "x": 5.2,
    "y": 3.1,
    "yaw": 1.57
  }
}
```

- `x`、`y`单位为米。
- `yaw`单位为弧度，从`+X`轴起逆时针为正。
- V1只允许单个导航目标。
- 急停激活、定位无效或车辆状态不允许时必须拒绝。

### 7.8 `cancel_mission`

取消当前任务时使用：

```json
{
  "data": {
    "mission_id": "mission_42"
  }
}
```

车辆网关必须验证 `mission_id` 是否与当前任务一致。V1暂不定义由地面站主动发起的暂停和继续命令；任务取消后如需再次导航，操作员必须重新下发新的导航命令。

---

## 8. 命令应答 `ack`

### 8.1 目的

地面站必须区分：

```text
命令已在本地创建
→ 命令已发送
→ 车辆已接受或拒绝
→ 车辆执行完成或失败
→ 地面站等待超时
```

WebSocket发送成功不代表车辆已接收，`accepted`也不代表车辆已经执行完成。

### 8.2 应答结构

```json
{
  "version": 1,
  "type": "ack",
  "name": "set_mode",
  "vehicle_id": "car_01",
  "seq": 90,
  "request_id": "cmd_1002",
  "timestamp": 1784651000250,
  "data": {
    "stage": "accepted",
    "code": "ok",
    "message": "mode change accepted"
  }
}
```

最终应答示例：

```json
{
  "version": 1,
  "type": "ack",
  "name": "set_mode",
  "vehicle_id": "car_01",
  "seq": 91,
  "request_id": "cmd_1002",
  "timestamp": 1784651000350,
  "data": {
    "stage": "completed",
    "code": "ok",
    "message": "mode changed to ground"
  }
}
```

### 8.3 应答阶段

| `stage` | 是否最终状态 | 含义 |
|---|---:|---|
| `accepted` | 否 | 命令合法并进入执行流程 |
| `rejected` | 是 | 命令未被接受，不会执行 |
| `completed` | 是 | 命令已经成功执行 |
| `failed` | 是 | 命令接受后执行失败 |

### 8.4 应答规则

- `ack.name`必须与原命令 `name`一致。
- `ack.request_id`必须与原命令一致。
- 要求应答的命令建议在局域网200毫秒内返回 `accepted` 或 `rejected`。
- `completed`和`failed`的最长等待时间由命令类型决定。
- 地面站超时是本地状态，不得伪造车辆端 `failed` 应答。
- 收到未知 `request_id` 的应答时，应记录诊断日志但不得改变其他命令状态。
- 对完全相同的重复请求，车辆端应返回缓存的原始最新阶段和原始`code`，不得再次执行命令。
- 同一 `request_id`携带不同命令或数据时，返回`rejected`和`duplicate_request`。

建议使用的 `code`：

| `code` | 含义 |
|---|---|
| `ok` | 正常 |
| `invalid_argument` | 参数无效 |
| `invalid_state` | 当前车辆状态不允许 |
| `mode_not_allowed` | 控制模式不允许 |
| `estop_active` | 急停激活导致拒绝 |
| `localization_unavailable` | 定位无效 |
| `mission_not_found` | 任务编号不存在 |
| `target_unreachable` | 导航目标不可达 |
| `execution_failed` | 执行失败 |
| `duplicate_request` | 同一编号被用于不同命令或不同数据 |

---

## 9. 状态事件 `event`

事件用于状态变化通知，不替代周期遥测。V1定义以下事件：

### 9.1 `gateway_ready`

车辆网关在连接建立并准备好处理消息后发送：

```json
{
  "version": 1,
  "type": "event",
  "name": "gateway_ready",
  "vehicle_id": "car_01",
  "seq": 1,
  "timestamp": 1784651000000,
  "data": {
    "gateway_version": "0.1.0",
    "gateway_instance_id": "gw_20260723_160241_a13f",
    "protocol_version": 1,
    "auto_disconnect_policy": "cancel_task_and_stop",
    "capabilities": [
      "set_mode",
      "set_estop",
      "clear_estop",
      "manual_control",
      "stop_motion",
      "navigate_to_pose",
      "cancel_mission"
    ]
  }
}
```

- `gateway_instance_id`由网关每次进程启动时生成，在本次进程生命周期内保持不变。该值变化表示网关已经重启。
- `auto_disconnect_policy`声明车辆处于 `auto` 模式时的WebSocket断联行为。V1固定为 `cancel_task_and_stop`，其他值均不受支持。
- 地面站不得启用车辆未声明支持的功能。
- 收到 `gateway_ready` 后，地面站仍必须等待新鲜的 `vehicle_status`，不得仅凭能力列表恢复控制操作。
- 如果 `gateway_instance_id`发生变化，地面站必须将旧实例所有未完成命令标记为“结果未知”，且不得自动重发。

### 9.2 `mode_changed`

```json
{
  "data": {
    "previous_mode": "auto",
    "current_mode": "ground",
    "reason": "command",
    "related_request_id": "cmd_1002"
  }
}
```

### 9.3 `mission_status`

```json
{
  "data": {
    "mission_id": "mission_42",
    "status": "running",
    "related_request_id": "cmd_1005",
    "progress_pct": 35,
    "message": "navigating to target"
  }
}
```

任务状态枚举：

- `idle`
- `waiting_confirmation`
- `running`
- `paused`
- `completed`
- `failed`
- `cancelled`
- `communication_lost`

`progress_pct`是可选整数，范围0～100。

### 9.4 `estop_changed`

```json
{
  "data": {
    "active": true,
    "reason": "operator_request",
    "related_request_id": "cmd_1003"
  }
}
```

急停激活和解除都必须发送事件。地面站仍需以后续实际遥测作为持续状态来源。

### 9.5 `manual_control_rejected`

高频人工控制不逐帧返回`ack`。当消息格式正确但车辆当前状态不允许执行时，网关应限频发送拒绝事件：

```json
{
  "data": {
    "related_seq": 104,
    "reason": "mode_not_allowed",
    "message": "manual control requires ground mode"
  }
}
```

`reason`建议使用`mode_not_allowed`、`estop_active`、`connection_not_ready`或`control_unavailable`。相同原因的事件应限频，避免高频控制造成事件洪水。

---

## 10. 车辆告警 `alarm`

告警表示车辆端检测到的重要异常或安全状态。

```json
{
  "version": 1,
  "type": "alarm",
  "name": "vehicle_alarm",
  "vehicle_id": "car_01",
  "seq": 120,
  "timestamp": 1784651010000,
  "data": {
    "alarm_id": "alarm_gps_0007",
    "code": "gps_fix_lost",
    "severity": "severe",
    "active": true,
    "message": "GPS定位失效",
    "first_timestamp": 1784651010000,
    "updated_timestamp": 1784651010000,
    "details": {}
  }
}
```

| 字段 | 类型 | 必需 | 说明 |
|---|---|---:|---|
| `alarm_id` | string | 是 | 一次告警实例的稳定编号 |
| `code` | string | 是 | 稳定的机器可读告警代码 |
| `severity` | string | 是 | `info`、`warning`、`severe`、`emergency` |
| `active` | boolean | 是 | `true`激活，`false`解除 |
| `message` | string | 是 | 面向操作员的说明 |
| `first_timestamp` | integer | 是 | 首次激活时间，Unix毫秒 |
| `updated_timestamp` | integer | 是 | 本次更新时间，Unix毫秒 |
| `details` | object | 否 | 告警特有的补充信息 |

规则：

- 同一告警从激活到解除必须保持相同 `alarm_id`。
- 告警解除时必须发送 `active:false`，不能依赖地面站自动清除。
- `severe`和`emergency`必须在主界面明显显示。
- `emergency`告警不等价于急停命令，但可能伴随 `estop_active:true`。

---

## 11. 协议错误 `error`

`error`用于报告协议层问题，不用于代替命令的 `rejected` 或 `failed` 应答。

```json
{
  "version": 1,
  "type": "error",
  "name": "protocol_error",
  "vehicle_id": "car_01",
  "seq": 121,
  "timestamp": 1784651010100,
  "data": {
    "code": "missing_field",
    "message": "command set_mode is missing data.mode",
    "related_seq": 102,
    "related_request_id": "cmd_1002",
    "details": {
      "field": "data.mode"
    }
  }
}
```

建议错误码：

| `code` | 含义 |
|---|---|
| `invalid_json` | JSON无法解析 |
| `unsupported_version` | 不支持的协议版本 |
| `unknown_message_type` | 未知消息类型 |
| `unknown_message_name` | 未知消息名称 |
| `missing_field` | 缺少必需字段 |
| `invalid_field_type` | 字段类型错误 |
| `invalid_field_value` | 字段值或范围错误 |
| `vehicle_id_mismatch` | 车辆编号不匹配 |
| `message_too_large` | 消息超过限制 |

如果输入文本连公共字段都无法解析，接收端可以只记录本地错误而不返回 `error`，避免形成错误消息循环。

收到 `error` 后不得自动把相关命令标记为执行成功。若错误关联命令，地面站应将其显示为发送或协议失败。

---

## 12. 安全要求

1. 人工运动命令只允许在车辆实际处于 `ground` 模式时执行。
2. 急停激活后，车辆端必须拒绝普通运动和开始导航命令。
3. 急停不得自动解除。
4. 车辆端必须独立实现人工控制500毫秒超时停车。
5. 地面站连接断开后必须停止普通控制发送并禁用相关操作。
6. 安全超时使用接收端本机单调时钟，不依赖消息 `timestamp`。
7. 具有副作用的命令使用 `request_id`防止重复执行。
8. 命令发送、接受、完成、失败、本地超时和结果未知必须分别记录。
9. 日志不得保存密码、令牌或高频完整原始报文。

### 12.1 WebSocket断联时的车辆行为

车辆网关检测到WebSocket断开或地面站控制超时后，必须根据车辆当时的实际模式独立执行以下规则，不得依赖地面站再发送停车命令：

| 实际模式 | 断联行为 |
|---|---|
| `ground` | 立即停止接收地面站人工控制；从检测到断联或控制超时起500毫秒内，将地面站控制对应的速度、油门和转向输出归零。 |
| `rc` | 不接管也不归零遥控器的有效控制输出，继续由遥控器控制；遥控链路失联后的行为由车辆端遥控链路保护机制负责。 |
| `auto` | 取消当前自动任务并停车；重连后不得恢复旧任务，只能由操作员重新下发新的导航任务。 |

V1的 `auto_disconnect_policy`固定为 `cancel_task_and_stop`。网关必须在 `gateway_ready` 中声明该值；未声明或声明为其他值时，地面站必须提示协议配置错误并禁用导航任务下发。

### 12.2 重连和网关重启

- 自动重连只恢复WebSocket链路，不代表之前的命令未执行，也不代表车辆已经恢复到断联前状态。
- 断联时未获得最终结果的导航、模式切换、解除急停等命令必须标记为“结果未知”，不得进入自动重发队列。
- 网关重启后，即使旧命令此前没有返回确认，地面站也不得自动重发。
- 地面站必须等待新的 `gateway_ready` 和 `vehicle_status`，以车辆实际上报的模式、急停和任务状态重建本地状态。
- 需要继续操作时，必须由操作员重新触发，并为新操作生成新的 `request_id`。
- `manual_control`只可在新遥测确认车辆处于 `ground`、急停未激活且操作员当前仍在主动输入时恢复发送；不得恢复断联前缓存的控制值。

---

## 13. 版本兼容规则

- V1消息固定使用 `"version":1`。
- 不支持的主版本必须返回 `unsupported_version`，随后可以关闭连接。
- 未知可选字段应忽略，以便同一主版本增加字段。
- 未知 `type` 或 `name`不得执行，应返回相应错误。
- 新增可选字段属于兼容变更。
- 删除字段、修改字段类型、单位、方向或既有枚举含义属于破坏性变更。
- 破坏性变更必须提升主版本并同步修改两端实现、测试和本文档。

---

## 14. 日志要求

地面站至少记录：

- WebSocket连接、断开、连接失败和自动重连。
- 心跳超时和恢复。
- JSON解析失败和协议验证失败。
- 未知消息类型和名称。
- 重要命令发送、接受、完成、拒绝、失败和超时。
- 模式、任务和急停状态变化。
- 告警激活和解除。

普通日志不得逐帧保存10Hz遥测或完整原始JSON。诊断日志可以保存消息类型、序号、大小和错误字段，但应避免保存无必要的完整负载。

---

## 15. V1测试要求

协议实现至少覆盖以下测试：

1. 正常 `ping` / `pong`及延迟计算。
2. 心跳超时和恢复。
3. 正常 `vehicle_status`。
4. 缺少公共字段。
5. 字段类型或范围错误。
6. 不支持的协议版本。
7. `vehicle_id`不匹配。
8. 未知 `type`和`name`。
9. 命令 `accepted`后`completed`。
10. 命令 `rejected`和`failed`。
11. 未知应答 `request_id`。
12. 重复 `request_id`不重复执行。
13. 急停时拒绝普通控制和导航。
14. 非`ground`模式拒绝人工控制。
15. 人工控制500毫秒超时停车。
16. 告警激活和解除使用相同 `alarm_id`。
17. 单条非法消息不会导致地面站崩溃。
18. `ground`、`rc`和`auto`模式下的断联行为符合第12节，且 `auto`模式会取消任务并停车。
19. 断联重连和网关重启后不自动重发未完成的副作用命令。
20. 网关实例变化后，旧命令被标记为“结果未知”，操作员重新操作时使用新的 `request_id`。

---

## 16. 待评审和确认事项

以下内容在网络实现前需要确认：

1. WebSocket URL是否需要固定路径，例如 `/ws`。
2. `steer_norm`负值左转、正值右转是否与车辆实际控制接口一致。
3. `gps_fix`枚举是否与车辆GPS驱动一致。
4. 哪些命令需要车辆网关在 `gateway_ready.capabilities` 中声明。
5. 模式切换、导航和急停各自的最终执行超时时间。

在上述事项确认前，本文档保持 Draft 状态。
