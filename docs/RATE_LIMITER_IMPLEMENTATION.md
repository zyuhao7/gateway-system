# 速率限制实现总结

## 实现内容

### 1. 核心算法

#### 令牌桶（Token Bucket）
- **原理**: 固定速率生成令牌，请求消耗令牌
- **特点**: 允许突发流量，平滑限流
- **参数**: 
  - `capacity`: 桶容量（最大令牌数）
  - `refill_rate`: 填充速率（令牌/秒）

#### 滑动窗口（Sliding Window）
- **原理**: 统计时间窗口内的请求数
- **特点**: 精确限流，防止边界问题
- **参数**:
  - `limit`: 限制数量
  - `window`: 时间窗口

### 2. 三种限流器

#### IP级别限流
```cpp
IPRateLimiter::instance().configure(100, 200);  // 100 req/s, burst 200
auto result = IPRateLimiter::instance().check("192.168.1.100");
```

**功能**:
- 防止单个IP的DDoS攻击
- IP封禁机制（临时/永久）
- 自动解封

#### 用户级别限流
```cpp
UserRateLimiter::instance().configure(1000, 1500);  // 1000 req/min, burst 1500
auto result = UserRateLimiter::instance().check("user_alice");
```

**功能**:
- 防止单个用户滥用API
- 按用户独立限流
- 支持不同用户等级

#### API端点限流
```cpp
APIRateLimiter::instance().configure("/api/search", 10, 20);  // 10 req/s, burst 20
auto result = APIRateLimiter::instance().check("/api/search", "client_123");
```

**功能**:
- 保护高成本API
- 不同API不同限制
- 按客户端独立计数

### 3. 测试结果

#### 7/7测试全部通过 ✅

```
✓ Test 1: 令牌桶算法 - 消耗、填充、拒绝
✓ Test 2: 滑动窗口 - 窗口限制、过期清理
✓ Test 3: 基础限流器 - 多key管理
✓ Test 4: IP限流器 - 限流、封禁、解封
✓ Test 5: 用户限流器 - 突发流量处理
✓ Test 6: API限流器 - 多端点独立限流
✓ Test 7: 并发访问 - 线程安全验证
```

## 使用示例

### 配置限流器

#### IP限流
```cpp
// 配置：100请求/秒，突发容量200
IPRateLimiter::instance().configure(100, 200);

// 检查IP
auto result = IPRateLimiter::instance().check("192.168.1.100");
if (!result.allowed) {
    return_error(429, "Too Many Requests");
}

// 封禁恶意IP（5分钟）
IPRateLimiter::instance().ban("192.168.1.100", std::chrono::minutes(5));
```

#### 用户限流
```cpp
// 配置：1000请求/分钟，突发1500
UserRateLimiter::instance().configure(1000, 1500);

// 检查用户
auto result = UserRateLimiter::instance().check("user_alice");
if (result.allowed) {
    process_request();
} else {
    return_error(429, result.error_message);
}
```

#### API限流
```cpp
// 配置搜索API：10请求/秒
APIRateLimiter::instance().configure("/api/search", 10, 20);

// 配置登录API：5请求/秒
APIRateLimiter::instance().configure("/api/login", 5, 10);

// 检查
auto result = APIRateLimiter::instance().check("/api/search", client_id);
```

### 集成到WebSocket

```cpp
void on_message(const std::string& ip, const std::string& user_id, const Message& msg) {
    // 1. IP限流
    auto ip_result = IPRateLimiter::instance().check(ip);
    if (!ip_result.allowed) {
        send_error("IP rate limit exceeded");
        return;
    }
    
    // 2. 用户限流
    auto user_result = UserRateLimiter::instance().check(user_id);
    if (!user_result.allowed) {
        send_error("User rate limit exceeded");
        return;
    }
    
    // 3. 处理消息
    process_message(msg);
}
```

### 响应头

```cpp
void set_rate_limit_headers(RateLimitResult result) {
    response.headers["X-RateLimit-Limit"] = "100";
    response.headers["X-RateLimit-Remaining"] = std::to_string(result.remaining);
    response.headers["X-RateLimit-Reset"] = std::to_string(result.reset_time);
    
    if (!result.allowed) {
        response.headers["Retry-After"] = std::to_string(result.reset_time - now());
    }
}
```

## 性能特性

### 时间复杂度
- **令牌桶**: O(1) - 常数时间
- **滑动窗口**: O(n) - n为窗口内请求数
- **查找限流器**: O(1) - 哈希表查找

### 空间复杂度
- **每个IP**: ~100字节
- **每个用户**: ~100字节
- **1万用户**: ~1MB内存

### 并发性能
- **线程安全**: 使用mutex保护
- **并发测试**: 10线程200次请求，正确限流
- **无死锁**: 细粒度锁

## 配置建议

### 典型场景配置

#### Web应用
```cpp
// IP限流：防DDoS
IPRateLimiter::instance().configure(100, 200);  // 100 req/s

// 用户限流：正常使用
UserRateLimiter::instance().configure(1000, 1500);  // 1000 req/min

// 登录API：防暴力破解
APIRateLimiter::instance().configure("/api/login", 5, 10);

// 搜索API：保护资源
APIRateLimiter::instance().configure("/api/search", 10, 20);
```

#### 游戏服务器
```cpp
// 玩家操作限流
UserRateLimiter::instance().configure(60, 100);  // 60 ops/min

// 房间创建限流
APIRateLimiter::instance().configure("/api/create_room", 1, 2);
```

#### IoT平台
```cpp
// 设备上报限流
UserRateLimiter::instance().configure(60, 120);  // 60 reports/min

// IP限流（防滥用）
IPRateLimiter::instance().configure(500, 1000);  // 500 req/s
```

## 监控指标

### Prometheus指标
```cpp
// 限流拒绝次数
rate_limit_rejected_total{type="ip"} 
rate_limit_rejected_total{type="user"} 
rate_limit_rejected_total{type="api"}

// 当前限流器数量
rate_limiters_active{type="ip"}

// 封禁IP数量
banned_ips_total
```

## 高级功能

### 动态调整
```cpp
// 根据负载动态调整
if (cpu_usage > 80%) {
    IPRateLimiter::instance().configure(50, 100);  // 降低限制
} else {
    IPRateLimiter::instance().configure(100, 200);  // 恢复正常
}
```

### 分级限流
```cpp
// VIP用户更高限额
if (user.is_vip()) {
    UserRateLimiter::instance().configure(5000, 10000);
} else {
    UserRateLimiter::instance().configure(1000, 1500);
}
```

### 清理过期数据
```cpp
// 定期清理（每小时）
std::thread cleanup_thread([]() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::hours(1));
        limiter.cleanup_expired(std::chrono::hours(24));
    }
});
```

## 限制与优化

### 当前限制
1. 单机内存存储（重启丢失）
2. 不支持分布式限流
3. 封禁列表不持久化

### 优化方向
- [ ] Redis存储（分布式）
- [ ] 持久化封禁列表
- [ ] 限流规则配置文件
- [ ] Grafana Dashboard
- [ ] 白名单机制

## 文件清单

- `include/rate_limiter.hpp` - 速率限制头文件
- `src/rate_limiter.cpp` - 速率限制实现
- `tests/test_rate_limiter.cpp` - 速率限制测试（7个测试）

## 依赖

- C++20 标准库
- `<chrono>` - 时间处理
- `<mutex>` - 线程安全
- `<deque>` - 滑动窗口存储
