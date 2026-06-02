# 用户认证实现总结

## 实现内容

### 1. JWT Token实现
- **算法**: HMAC-SHA256签名
- **编码**: Base64 URL安全编码
- **结构**: Header.Payload.Signature

### 2. Token字段
```json
{
  "header": {
    "alg": "HS256",
    "typ": "JWT"
  },
  "payload": {
    "user_id": "alice",
    "iat": 1780382804,  // 签发时间
    "exp": 1780469204,  // 过期时间
    "claims": {}        // 自定义声明
  }
}
```

### 3. 核心功能
- ✅ **Token生成** - 使用HMAC-SHA256签名
- ✅ **Token验证** - 签名验证 + 过期检查
- ✅ **Token吊销** - 黑名单机制
- ✅ **Token刷新** - 吊销旧Token + 生成新Token
- ✅ **自动清理** - 清理过期的黑名单Token

### 4. 安全特性
- **签名验证**: 防止Token篡改
- **过期检查**: 自动拒绝过期Token
- **黑名单机制**: 主动吊销Token
- **URL安全编码**: Base64 URL-safe

## 测试结果

### 7个测试全部通过 ✅

```
✓ Test 1: JWT Token生成
✓ Test 2: JWT Token验证
✓ Test 3: 无效Token处理
✓ Test 4: Token过期
✓ Test 5: Token吊销
✓ Test 6: Token刷新
✓ Test 7: JWTAuthenticator
```

## 使用示例

### 服务端生成Token
```cpp
#include "auth.hpp"

// 设置密钥
AuthManager::instance().set_secret("your_secret_key_here");

// 生成Token（默认24小时有效）
std::string token = AuthManager::instance().generate_token("user_alice");

// 自定义过期时间（1小时）
std::string token = AuthManager::instance().generate_token(
    "user_bob",
    std::chrono::hours(1)
);
```

### 服务端验证Token
```cpp
// 验证Token
auto user_id = AuthManager::instance().verify_token(token);

if (user_id) {
    std::cout << "Authenticated user: " << *user_id << std::endl;
    // 允许访问
} else {
    std::cout << "Authentication failed" << std::endl;
    // 拒绝访问
}
```

### 吊销Token
```cpp
// 用户登出时吊销Token
AuthManager::instance().revoke_token(token);

// 后续验证会失败
auto result = AuthManager::instance().verify_token(token);
// result == std::nullopt
```

### 刷新Token
```cpp
// 刷新Token（续期）
auto new_token = AuthManager::instance().refresh_token(old_token);

if (new_token) {
    // 旧Token自动失效
    // 新Token有效期延长
    return *new_token;
}
```

### 使用JWTAuthenticator
```cpp
JWTAuthenticator auth("your_secret_key");

AuthResult result = auth.authenticate(token);

if (result.success) {
    std::cout << "User: " << result.user_id << std::endl;
} else {
    std::cout << "Error: " << result.error_message << std::endl;
}
```

## 集成到WebSocket

### 连接时认证
```cpp
// 客户端发送连接消息
{
  "type": "connect",
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}

// 服务端验证
void on_connect(const std::string& token) {
    auto user_id = AuthManager::instance().verify_token(token);
    
    if (!user_id) {
        // 认证失败，关闭连接
        send_error("Authentication failed");
        close_connection();
        return;
    }
    
    // 认证成功，保存用户信息
    connection->set_user_id(*user_id);
}
```

## Token格式示例

```
eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.
eyJ1c2VyX2lkIjoidXNlcl9hbGljZSIsImlhdCI6MTc4MDM4MjgwNCwiZXhwIjoxNzgwNDY5MjA0fQ.
-0BTYzXhV7Hjl9H05X89JrsZlyJ77b2sBAllk8jAWdo
```

**解码后**:

Header:
```json
{"alg":"HS256","typ":"JWT"}
```

Payload:
```json
{
  "user_id":"user_alice",
  "iat":1780382804,
  "exp":1780469204
}
```

## 性能特性

- **签名速度**: HMAC-SHA256 非常快
- **验证速度**: 毫秒级
- **内存占用**: 极小（只存储黑名单）
- **无状态**: Token自包含，无需查数据库

## 安全建议

1. **密钥管理**
   - 使用强随机密钥（至少32字节）
   - 定期轮换密钥
   - 不要硬编码密钥

2. **Token生命周期**
   - 访问Token: 短期（15分钟-1小时）
   - 刷新Token: 长期（7天-30天）
   - 根据安全需求调整

3. **传输安全**
   - 使用HTTPS/WSS传输
   - 不要在URL中传递Token
   - 使用Authorization header

4. **黑名单清理**
   - 定期清理过期Token
   - 考虑使用Redis存储黑名单
   - 设置合理的过期时间

## 后续优化

- [ ] 支持RS256（非对称加密）
- [ ] 集成Redis存储黑名单（分布式）
- [ ] 添加刷新Token机制（双Token）
- [ ] 支持自定义Claims
- [ ] 添加Token版本号（强制失效所有旧Token）

## 文件清单

- `include/auth.hpp` - 认证模块头文件
- `src/auth.cpp` - 认证模块实现
- `tests/test_auth.cpp` - 认证测试

## 依赖库

- OpenSSL - HMAC-SHA256签名
- Boost.JSON - JSON序列化
- Boost.Beast - Base64编码
