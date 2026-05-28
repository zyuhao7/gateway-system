# 即时通讯（IM）示例

基于网关系统的即时通讯应用示例。

## 功能特性

### 核心功能
- ✅ 用户连接和Token认证
- ✅ 一对一聊天
- ✅ 群组聊天
- ✅ 离线消息存储
- ✅ 历史消息查询
- ✅ 消息送达确认（ACK）
- ✅ 在线状态管理

### 消息类型
- **CONNECT**: 用户连接（携带token）
- **MESSAGE**: 一对一消息
- **GROUP_MESSAGE**: 群组消息
- **ACK**: 消息确认
- **HEARTBEAT**: 心跳保活
- **QUERY_HISTORY**: 查询历史消息
- **QUERY_OFFLINE**: 查询离线消息

## 架构设计

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ WebSocket
┌──────▼──────┐
│ IM Handler  │ ← 消息处理
├─────────────┤
│  Protocol   │ ← IM协议
├─────────────┤
│   Storage   │ ← 消息存储
├─────────────┤
│   Gateway   │ ← 核心网关
└─────────────┘
```

## 协议定义

### 连接消息
```json
{
  "type": "connect",
  "user_id": "alice",
  "token": "jwt_token_here",
  "client_info": {
    "platform": "web",
    "version": "1.0.0"
  }
}
```

### 聊天消息
```json
{
  "type": "message",
  "message_id": "msg_123",
  "from_user": "alice",
  "to_user": "bob",
  "content": "Hello!",
  "timestamp": 1779937656025040,
  "require_ack": true
}
```

### ACK确认
```json
{
  "type": "ack",
  "ack_id": "msg_123",
  "success": true
}
```

## 使用示例

### 编译
```bash
cd gateway-system
cmake -B build -DBUILD_EXAMPLE_IM=ON
cmake --build build --target im_server
```

### 运行服务器
```bash
# 启动IM服务器（端口8080，4线程）
./build/examples/im/im_server 8080 4

# 指定Redis和数据库路径
./build/examples/im/im_server 8080 4 \
  --redis=localhost:6379 \
  --db=/data/im_messages
```

### 客户端连接

#### JavaScript客户端
```javascript
const ws = new WebSocket('ws://localhost:8080');

// 连接
ws.onopen = () => {
  ws.send(JSON.stringify({
    type: 'connect',
    user_id: 'alice',
    token: 'your_jwt_token'
  }));
};

// 发送消息
function sendMessage(to, content) {
  ws.send(JSON.stringify({
    type: 'message',
    to_user: to,
    content: content,
    require_ack: true
  }));
}

// 接收消息
ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  
  if (msg.type === 'message') {
    console.log(`${msg.from_user}: ${msg.content}`);
    
    // 发送ACK
    ws.send(JSON.stringify({
      type: 'ack',
      ack_id: msg.message_id,
      success: true
    }));
  }
};
```

#### Python客户端
```python
import websocket
import json

def on_message(ws, message):
    msg = json.loads(message)
    
    if msg['type'] == 'message':
        print(f"{msg['from_user']}: {msg['content']}")
        
        # 发送ACK
        ws.send(json.dumps({
            'type': 'ack',
            'ack_id': msg['message_id'],
            'success': True
        }))

ws = websocket.WebSocketApp(
    'ws://localhost:8080',
    on_message=on_message
)

# 连接
ws.on_open = lambda ws: ws.send(json.dumps({
    'type': 'connect',
    'user_id': 'alice',
    'token': 'your_jwt_token'
}))

ws.run_forever()
```

## 测试

### 单元测试
```bash
./build/examples/im/test_im_protocol
./build/examples/im/test_im_handler
./build/examples/im/test_im_storage
```

### 集成测试
```bash
# 启动服务器
./build/examples/im/im_server 8080 4 &

# 运行测试客户端
./build/examples/im/test_im_client localhost 8080
```

### 压测
```bash
# 1000个并发连接
./build/examples/im/im_stress_test localhost 8080 1000
```

## 性能指标

### 测试环境
- CPU: 8核
- 内存: 16GB
- 网络: 本地回环

### 测试结果
| 连接数 | 消息吞吐 | P99延迟 | 内存占用 | CPU使用 |
|--------|----------|---------|----------|---------|
| 1K | 5K msg/s | 5ms | 50MB | 5% |
| 10K | 10K msg/s | 8ms | 82MB | 15% |
| 50K | 15K msg/s | 12ms | 400MB | 40% |

## 存储容量

### 消息存储
- 平均消息大小: 500字节
- 压缩率: 50%
- 每天1000万条消息: 2.5GB

### 清理策略
- 已送达消息保留30天
- 未送达消息保留90天
- 自动清理任务每天凌晨执行

## 扩展功能

### 群组聊天
```cpp
// 创建群组
GroupMessage create_group;
create_group.group_id = "group_123";
create_group.members = {"alice", "bob", "charlie"};

// 发送群组消息
GroupMessage msg;
msg.group_id = "group_123";
msg.from_user = "alice";
msg.content = "Hello everyone!";
```

### 消息加密
```cpp
// 启用端到端加密
EncryptedMessage msg;
msg.encrypted_content = encrypt(content, recipient_public_key);
msg.encryption_type = "RSA-2048";
```

### 富文本消息
```json
{
  "type": "message",
  "content_type": "rich_text",
  "content": {
    "text": "Check this out!",
    "attachments": [
      {
        "type": "image",
        "url": "https://example.com/image.jpg"
      }
    ]
  }
}
```

## 配置文件

`config/im_config.json`:
```json
{
  "server": {
    "port": 8080,
    "threads": 4,
    "max_connections": 100000
  },
  "storage": {
    "type": "rocksdb",
    "path": "/data/im_messages",
    "retention_days": 30
  },
  "redis": {
    "host": "localhost",
    "port": 6379
  },
  "auth": {
    "jwt_secret": "your_secret_key",
    "token_expire": 86400
  }
}
```

## 监控指标

### Prometheus指标
- `im_messages_sent_total` - 发送消息总数
- `im_messages_received_total` - 接收消息总数
- `im_offline_messages` - 离线消息数
- `im_active_users` - 在线用户数
- `im_message_latency` - 消息延迟

### Grafana Dashboard
导入 `grafana/im-dashboard.json` 查看实时监控。

## 故障排查

### 消息丢失
1. 检查ACK机制是否正常
2. 查看存储日志
3. 验证网络连接

### 延迟过高
1. 检查CPU和内存使用
2. 增加工作线程数
3. 优化数据库查询

### 连接断开
1. 检查心跳配置
2. 查看网络状态
3. 验证Token有效期

## 参考资料

- [IM协议规范](./docs/IM_PROTOCOL.md)
- [存储设计](./docs/IM_STORAGE.md)
- [性能优化](./docs/IM_PERFORMANCE.md)
