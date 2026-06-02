/**
 * @file auth.hpp
 * @brief 用户认证模块
 */

#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <map>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <boost/json.hpp>
#include <boost/beast/core/detail/base64.hpp>

namespace gateway {

namespace json = boost::json;

// JWT Token结构
struct JWTToken {
    // Header
    std::string algorithm = "HS256";
    std::string type = "JWT";

    // Payload
    std::string user_id;
    int64_t issued_at;      // iat: 签发时间
    int64_t expires_at;     // exp: 过期时间
    std::map<std::string, std::string> claims;  // 自定义声明

    // 生成Token字符串
    std::string generate(const std::string& secret) const;

    // 验证Token
    static std::optional<JWTToken> verify(const std::string& token, const std::string& secret);

private:
    static std::string base64_url_encode(const std::string& input);
    static std::string base64_url_decode(const std::string& input);
    static std::string hmac_sha256(const std::string& data, const std::string& secret);
};

// 认证管理器
class AuthManager {
public:
    static AuthManager& instance();

    // 设置密钥
    void set_secret(const std::string& secret);

    // 生成Token
    std::string generate_token(const std::string& user_id,
                               std::chrono::seconds ttl = std::chrono::hours(24));

    // 验证Token
    std::optional<std::string> verify_token(const std::string& token);

    // 吊销Token（黑名单）
    void revoke_token(const std::string& token);

    // 检查是否被吊销
    bool is_revoked(const std::string& token);

    // 刷新Token
    std::optional<std::string> refresh_token(const std::string& old_token);

private:
    AuthManager() = default;

    std::string secret_;
    std::map<std::string, int64_t> revoked_tokens_;  // token -> 过期时间

    void cleanup_expired_tokens();
};

// 认证结果
struct AuthResult {
    bool success;
    std::string user_id;
    std::string error_message;

    static AuthResult ok(const std::string& user_id) {
        return {true, user_id, ""};
    }

    static AuthResult fail(const std::string& error) {
        return {false, "", error};
    }
};

// 认证器接口
class Authenticator {
public:
    virtual ~Authenticator() = default;

    // 认证连接
    virtual AuthResult authenticate(const std::string& token) = 0;
};

// JWT认证器
class JWTAuthenticator : public Authenticator {
public:
    JWTAuthenticator(const std::string& secret);

    AuthResult authenticate(const std::string& token) override;

private:
    std::string secret_;
};

}
