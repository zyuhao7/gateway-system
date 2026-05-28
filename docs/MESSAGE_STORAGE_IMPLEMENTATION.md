# 消息持久化实现总结

## 实现内容

### 1. 存储结构
```cpp
struct StoredMessage {
    std::string message_id;      // 消息ID
    std::string from_user;       // 发送者
    std::string to_user;         // 接收者
    std::string content;         // 内容
    int64_t timestamp;           // 时间戳
    bool delivered;              // 是否已送达
};
```

### 2. 核心功能

#### 消息存储
- **store_message()**: 存储消息到RocksDB
- 双索引设计:
  - `msg:{message_id}` → 消息内容
  - `user:{user_id}:{timestamp}` → message_id

#### 离线消息
- **get_offline_messages()**: 获取用户的未送达消息
- 扫描用户索引，过滤未送达消息

#### 消息确认
- **mark_delivered()**: 标记消息为已送达
- 更新消息的delivered字段

#### 历史查询
- **get_history()**: 按时间范围查询历史消息
- 支持limit限制返回数量

#### 清理
- **cleanup_old_messages()**: 删除已送达的旧消息
- 按时间戳过滤，释放存储空间

### 3. 数据库配置
- **引擎**: RocksDB 8.9.1
- **压缩**: Snappy压缩
- **自动创建**: create_if_missing = true

## 测试结果

### 功能测试
```
✅ Test 1: 打开数据库 - 通过
✅ Test 2: 存储消息 - 通过
✅ Test 3: 获取离线消息 - 通过（2条）
✅ Test 4: 标记已送达 - 通过（剩余1条）
✅ Test 5: 历史查询 - 通过（2条）
✅ Test 6: 清理旧消息 - 通过（删除1条）
✅ Test 7: 关闭数据库 - 通过
```

## 存储示例

### 消息键值对

**消息内容**:
```
Key: msg:msg001
Value: {
  "message_id": "msg001",
  "from_user": "user_alice",
  "to_user": "user_bob",
  "content": "Hello Bob!",
  "timestamp": 1779937656025040,
  "delivered": false
}
```

**用户索引**:
```
Key: user:user_bob:1779937656025040
Value: msg001
```

## 使用示例

### 存储消息
```cpp
auto& storage = MessageStorage::instance();
storage.open("/data/messages");

StoredMessage msg;
msg.message_id = "msg001";
msg.from_user = "alice";
msg.to_user = "bob";
msg.content = "Hello!";
msg.timestamp = Message::current_timestamp();
msg.delivered = false;

storage.store_message(msg);
```

### 获取离线消息
```cpp
auto offline = storage.get_offline_messages("bob");

for (const auto& msg : offline) {
    // 发送给用户
    send_to_user(msg);
    
    // 标记为已送达
    storage.mark_delivered(msg.message_id);
}
```

### 查询历史
```cpp
int64_t start = now - 7 * 24 * 3600 * 1000000; // 7天前
int64_t end = now;

auto history = storage.get_history("bob", start, end, 100);

for (const auto& msg : history) {
    std::cout << msg.from_user << ": " << msg.content << std::endl;
}
```

### 定期清理
```cpp
// 删除30天前的已送达消息
int64_t cutoff = now - 30 * 24 * 3600 * 1000000;
size_t deleted = storage.cleanup_old_messages(cutoff);
std::cout << "Cleaned up " << deleted << " messages" << std::endl;
```

## 性能特性

1. **快速写入**: RocksDB LSM-Tree架构，写入性能优异
2. **范围查询**: 支持按时间范围高效查询
3. **压缩存储**: Snappy压缩，节省磁盘空间
4. **索引优化**: 双索引设计，快速定位用户消息
5. **批量操作**: 支持批量读取和删除

## 存储容量估算

假设：
- 平均消息大小: 500 字节
- 压缩率: 50%
- 每天消息量: 1000万条

**存储需求**:
- 每天: 1000万 × 500字节 × 50% = 2.5 GB
- 每月: 2.5 GB × 30 = 75 GB
- 每年: 75 GB × 12 = 900 GB

## 优化建议

1. **分区存储**: 按用户或时间分区，提高查询效率
2. **批量写入**: 使用WriteBatch批量写入
3. **异步清理**: 后台线程定期清理旧消息
4. **缓存层**: 热点数据缓存到Redis
5. **备份策略**: 定期备份RocksDB数据

## 集成到网关

### 消息发送时存储
```cpp
// 发送消息前存储
StoredMessage stored;
stored.message_id = msg.message_id;
stored.from_user = msg.from_user;
stored.to_user = msg.to_user;
stored.content = msg.content;
stored.timestamp = msg.timestamp;
stored.delivered = false;

MessageStorage::instance().store_message(stored);

// 尝试发送
if (send_to_user(msg)) {
    // 立即送达，标记
    MessageStorage::instance().mark_delivered(msg.message_id);
} else {
    // 用户离线，消息已存储
}
```

### 用户上线时推送
```cpp
void on_user_connect(const std::string& user_id) {
    // 获取离线消息
    auto offline = MessageStorage::instance().get_offline_messages(user_id);
    
    for (const auto& msg : offline) {
        // 推送给用户
        send_message(user_id, msg);
        
        // 标记已送达
        MessageStorage::instance().mark_delivered(msg.message_id);
    }
}
```

## 下一步

- [ ] 集成到WebSocket连接
- [ ] 实现消息队列
- [ ] 添加消息加密存储
- [ ] 实现消息搜索
- [ ] 添加消息统计
