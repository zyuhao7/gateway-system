# 应用示例说明

本目录包含基于网关系统的多种应用场景示例，展示网关的通用性和灵活性。

## 示例列表

### 1. 即时通讯（IM）
**目录**: `examples/im/`

**功能**:
- 用户连接和认证
- 一对一聊天
- 群组聊天
- 离线消息存储
- 历史消息查询
- 消息确认机制

**适用场景**:
- 社交应用
- 企业通讯
- 客服系统

### 2. 游戏服务器
**目录**: `examples/game/`

**功能**:
- 玩家连接管理
- 房间/匹配系统
- 实时状态同步
- 帧同步
- 战斗结算

**适用场景**:
- 多人在线游戏
- 实时对战游戏
- 棋牌游戏

### 3. 物联网（IoT）
**目录**: `examples/iot/`

**功能**:
- 设备注册和认证
- 数据采集和上报
- 指令下发
- 设备状态监控
- 数据持久化

**适用场景**:
- 智能家居
- 工业物联网
- 车联网

### 4. 推送服务
**目录**: `examples/push/`

**功能**:
- 订阅管理
- 消息推送
- 标签分组
- 定时推送
- 推送统计

**适用场景**:
- 消息通知
- 营销推送
- 系统告警

## 架构说明

### 核心网关（通用）
```
src/
├── server.cpp              # TCP服务器
├── websocket_server.cpp    # WebSocket服务器
├── connection_manager.cpp  # 连接管理
├── message_router.cpp      # 消息路由
├── consistent_hash.cpp     # 一致性哈希
└── metrics.cpp            # 监控指标
```

### 示例应用（专用）
```
examples/[场景]/
├── include/
│   ├── [场景]_protocol.hpp    # 专用协议
│   ├── [场景]_handler.hpp     # 消息处理
│   └── [场景]_storage.hpp     # 存储逻辑
├── src/
│   ├── [场景]_handler.cpp
│   └── [场景]_storage.cpp
├── tests/
│   └── test_[场景].cpp
└── README.md
```

## 使用方法

### 编译示例

```bash
# 编译IM示例
cmake -B build -DBUILD_EXAMPLE_IM=ON
cmake --build build --target im_server

# 编译游戏示例
cmake -B build -DBUILD_EXAMPLE_GAME=ON
cmake --build build --target game_server

# 编译所有示例
cmake -B build -DBUILD_ALL_EXAMPLES=ON
cmake --build build
```

### 运行示例

```bash
# 运行IM服务器
./build/examples/im/im_server 8080 4

# 运行游戏服务器
./build/examples/game/game_server 8081 8

# 运行IoT服务器
./build/examples/iot/iot_server 8082 4

# 运行推送服务器
./build/examples/push/push_server 8083 4
```

## 自定义应用

基于网关系统开发自定义应用的步骤：

### 1. 定义协议
```cpp
// my_protocol.hpp
#include "protocol.hpp"

struct MyMessage : public Message {
    // 自定义字段
    std::string custom_field;
    
    // 序列化
    std::string serialize() const override;
    static std::optional<MyMessage> deserialize(const std::string& data);
};
```

### 2. 实现处理器
```cpp
// my_handler.cpp
#include "my_protocol.hpp"

class MyHandler {
public:
    void handle_message(const MyMessage& msg) {
        // 处理消息逻辑
    }
};
```

### 3. 集成到网关
```cpp
// my_server.cpp
#include "websocket_server.hpp"
#include "my_handler.hpp"

int main() {
    WebSocketServer server(io_context, 8080);
    MyHandler handler;
    
    // 注册消息处理器
    server.set_message_handler([&](const std::string& msg) {
        auto parsed = MyMessage::deserialize(msg);
        if (parsed) {
            handler.handle_message(*parsed);
        }
    });
    
    server.listen();
}
```

## 性能对比

| 场景 | 连接数 | 消息吞吐 | 延迟 | 内存 |
|------|--------|----------|------|------|
| IM | 10K | 10K msg/s | <10ms | 82MB |
| 游戏 | 5K | 50K msg/s | <5ms | 120MB |
| IoT | 50K | 5K msg/s | <20ms | 200MB |
| 推送 | 100K | 1K msg/s | <50ms | 400MB |

## 扩展性

网关系统支持：
- **水平扩展**: 通过一致性哈希分布连接
- **垂直扩展**: 多线程处理提高单机性能
- **协议扩展**: 支持TCP、WebSocket、可扩展MQTT等
- **存储扩展**: 支持RocksDB、Redis、MySQL等

## 贡献示例

欢迎贡献新的应用场景示例！

提交PR时请包含：
1. 完整的示例代码
2. README说明文档
3. 单元测试
4. 性能测试报告

## 参考资料

- [核心架构文档](../docs/ARCHITECTURE.md)
- [协议规范](../docs/PROTOCOL_IMPLEMENTATION.md)
- [性能优化](../docs/PERFORMANCE.md)
