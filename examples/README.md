# 应用示例

本目录包含4个完整可运行的应用场景示例，展示网关系统在不同领域的应用。

## 示例列表

### 1. IM服务器 (即时通讯)
**目录**: `im/`  
**可执行文件**: `build/examples/im/im_server`

**功能演示**:
- √ 用户注册和JWT认证
- √ 创建聊天室
- √ 多人群聊
- √ 离线消息存储
- √ 历史消息查询
- √ 速率限制

**运行**:
```bash
mkdir -p data/im_messages
./build/examples/im/im_server
```

**输出示例**:
```
=== IM Server Demo ===
[1] User Registration
  Alice token: eyJhbGci...
[2] User Connection
  Alice connected (verified: alice)
[3] Create Room
  Room created: room_xxx
[4] Join Room
  Bob joined room
[5] Send Messages
  alice: Hello everyone!
```

---

### 2. 游戏服务器
**目录**: `game/`  
**可执行文件**: `build/examples/game/game_server`

**功能演示**:
- √ 玩家匹配队列
- √ 房间管理（Battle Royale模式）
- √ 游戏状态同步
- √ 分数统计
- √ 房间清理

**运行**:
```bash
./build/examples/game/game_server
```

**输出示例**:
```
=== Game Server Demo ===
[1] Players Entering Matchmaking
  4 players joined
[2] Match Found - Creating Room
  Room ID: room_xxx
  Mode: Battle Royale
[3] Game Starting
  Map: Desert Arena
[4] Game In Progress
  Round 1: player1 scored 428 points
```

---

### 3. IoT服务器 (物联网)
**目录**: `iot/`  
**可执行文件**: `build/examples/iot/iot_server`

**功能演示**:
- √ 设备注册管理
- √ 设备上线/离线
- √ 传感器数据采集（温度、湿度、运动检测）
- √ 告警检测
- √ 设备控制
- √ 数据持久化

**运行**:
```bash
mkdir -p data/iot_data
./build/examples/iot/iot_server
```

**输出示例**:
```
=== IoT Server Demo ===
[1] Device Registration
  Device registered: device_001 (temperature_sensor)
[3] Data Collection
  device_001: 25.4°C
  device_002: 66.3%
  device_003: Motion detected
[4] Alert Detection
  ⚠️ Alert: High temperature detected!
```

---

### 4. 推送服务器
**目录**: `push/`  
**可执行文件**: `build/examples/push/push_server`

**功能演示**:
- √ 主题订阅
- √ 标签分组
- √ 主题推送
- √ 标签推送
- √ 单播推送
- √ 广播推送
- √ 定时推送
- √ 推送统计

**运行**:
```bash
./build/examples/push/push_server
```

**输出示例**:
```
=== Push Server Demo ===
[1] User Subscriptions
  user_001 subscribed to: news, sports
[2] Topic Push
  Pushing to topic 'news'
  Delivered: 2/2
[3] Tag-based Push
  Pushing to tags: vip
  Delivered: 1/1
```

---

## 编译所有示例

```bash
# 配置CMake（启用示例）
cmake -B build -DBUILD_EXAMPLES=ON

# 编译所有示例
cmake --build build -j$(nproc)

# 单独编译某个示例
cmake --build build --target im_server
cmake --build build --target game_server
cmake --build build --target iot_server
cmake --build build --target push_server
```

## 技术栈

所有示例使用相同的核心模块：

| 模块 | IM | 游戏 | IoT | 推送 |
|------|----|----|-----|------|
| 认证 (Auth) | √ | - | - | - |
| 速率限制 (RateLimiter) | √ | √ | √ | √ |
| 房间管理 (Room) | √ | √ | - | - |
| 协议 (Protocol) | √ | - | - | - |
| 消息存储 (MessageStorage) | √ | - | √ | - |

## 示例特点

### 代码简洁
- 每个示例 ~200-300行代码
- 清晰的模块调用
- 完整的功能演示

### 独立运行
- 无需网络连接
- 模拟真实场景
- 即时看到效果

### 易于扩展
- 可作为实际项目起点
- 模块化设计便于修改
- 详细的注释说明

## 自定义示例

### 修改端口
```bash
./build/examples/im/im_server 9000  # 使用端口9000
```

### 修改配置
编辑对应的 `.cpp` 文件：

```cpp
// IM服务器配置
IPRateLimiter::instance().configure(100, 200);      // IP限流
UserRateLimiter::instance().configure(1000, 1500);  // 用户限流

// 游戏服务器配置
room->set_max_members(4);  // 最大玩家数

// IoT服务器配置
IPRateLimiter::instance().configure(500, 1000);  // 设备限流

// 推送服务器配置
UserRateLimiter::instance().configure(1000, 1500);  // 推送限流
```

## 实际应用

这些示例可以作为以下场景的基础：

### IM服务器
- 企业内部通讯工具
- 在线客服系统
- 社交聊天应用
- 协作工具

### 游戏服务器
- 多人在线游戏
- 实时对战游戏
- 房间匹配系统
- 游戏大厅

### IoT服务器
- 智能家居平台
- 工业监控系统
- 环境监测
- 设备管理平台

### 推送服务器
- 移动应用推送
- 邮件通知系统
- 营销推送平台
- 事件提醒服务

## 性能参考

基于10K连接压测结果：

| 指标 | 数值 |
|------|------|
| 并发连接 | 10,000+ |
| 内存占用 | ~82MB |
| CPU使用 | ~0.8% |
| 消息延迟 | <10ms (P99) |
| 吞吐量 | 10K+ msg/s |

## 下一步

1. **学习**: 阅读示例代码，理解模块调用
2. **修改**: 根据需求调整配置和逻辑
3. **扩展**: 添加新功能或集成WebSocket
4. **部署**: 集成到实际项目中使用

## 问题反馈

如有问题或建议，请提交Issue：
https://github.com/zyuhao7/gateway-system/issues
