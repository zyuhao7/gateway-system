/**
 * @file auth.cpp
 * @brief 用户认证实现
 */

#include "auth.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <thread>

namespace gateway {

// Base64 URL编码
std::string JWTToken::base64_url_encode(const std::string& input) {
    std::string base64;
    base64.resize(boost::beast::detail::base64::encoded_size(input.size()));

    auto written = boost::beast::detail::base64::encode(
        base64.data(),
        input.data(),
        input.size()
    );
    base64.resize(written);

    // URL安全：替换字符
    for (auto& c : base64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }

    // 移除padding
    base64.erase(std::find(base64.begin(), base64.end(), '='), base64.end());

    return base64;
}

// Base64 URL解码
std::string JWTToken::base64_url_decode(const std::string& input) {
    std::string base64 = input;

    // 还原URL安全字符
    for (auto& c : base64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }

    // 添加padding
    while (base64.size() % 4 != 0) {
        base64 += '=';
    }

    std::string output;
    output.resize(boost::beast::detail::base64::decoded_size(base64.size()));

    auto written = boost::beast::detail::base64::decode(
        output.data(),
        base64.data(),
        base64.size()
    );

    output.resize(written.first);
    return output;
}

// HMAC-SHA256签名
std::string JWTToken::hmac_sha256(const std::string& data, const std::string& secret) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    HMAC(
        EVP_sha256(),
        secret.data(), secret.size(),
        reinterpret_cast<const unsigned char*>(data.data()), data.size(),
        hash, &hash_len
    );

    return std::string(reinterpret_cast<char*>(hash), hash_len);
}

// 生成Token
std::string JWTToken::generate(const std::string& secret) const {
    // Header
    json::object header;
    header["alg"] = algorithm;
    header["typ"] = type;
    std::string header_str = json::serialize(header);

    // Payload
    json::object payload;
    payload["user_id"] = user_id;
    payload["iat"] = issued_at;
    payload["exp"] = expires_at;

    for (const auto& [key, value] : claims) {
        payload[key] = value;
    }
    std::string payload_str = json::serialize(payload);

    // Base64编码
    std::string header_b64 = base64_url_encode(header_str);
    std::string payload_b64 = base64_url_encode(payload_str);

    // 签名
    std::string signature_input = header_b64 + "." + payload_b64;
    std::string signature = hmac_sha256(signature_input, secret);
    std::string signature_b64 = base64_url_encode(signature);

    // 组装Token
    return header_b64 + "." + payload_b64 + "." + signature_b64;
}

// 验证Token
std::optional<JWTToken> JWTToken::verify(const std::string& token, const std::string& secret) {
    // 分割Token
    size_t pos1 = token.find('.');
    size_t pos2 = token.find('.', pos1 + 1);

    if (pos1 == std::string::npos || pos2 == std::string::npos) {
        return std::nullopt;
    }

    std::string header_b64 = token.substr(0, pos1);
    std::string payload_b64 = token.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string signature_b64 = token.substr(pos2 + 1);

    // 验证签名
    std::string signature_input = header_b64 + "." + payload_b64;
    std::string expected_signature = hmac_sha256(signature_input, secret);
    std::string expected_signature_b64 = base64_url_encode(expected_signature);

    if (signature_b64 != expected_signature_b64) {
        return std::nullopt;  // 签名不匹配
    }

    // 解码Payload
    std::string payload_str = base64_url_decode(payload_b64);

    try {
        auto jv = json::parse(payload_str);
        auto& obj = jv.as_object();

        JWTToken jwt;
        jwt.user_id = std::string(obj["user_id"].as_string());
        jwt.issued_at = obj["iat"].as_int64();
        jwt.expires_at = obj["exp"].as_int64();

        // 检查过期（时间戳是秒）
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        if (jwt.expires_at < now) {
            return std::nullopt;  // Token已过期
        }

        return jwt;

    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// AuthManager实现
AuthManager& AuthManager::instance() {
    static AuthManager manager;
    return manager;
}

void AuthManager::set_secret(const std::string& secret) {
    secret_ = secret;
}

std::string AuthManager::generate_token(const std::string& user_id, std::chrono::seconds ttl) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    JWTToken jwt;
    jwt.user_id = user_id;
    jwt.issued_at = now_sec;
    jwt.expires_at = now_sec + ttl.count();

    return jwt.generate(secret_);
}

std::optional<std::string> AuthManager::verify_token(const std::string& token) {
    // 检查是否被吊销
    if (is_revoked(token)) {
        return std::nullopt;
    }

    auto jwt = JWTToken::verify(token, secret_);
    if (!jwt) {
        return std::nullopt;
    }

    return jwt->user_id;
}

void AuthManager::revoke_token(const std::string& token) {
    auto jwt = JWTToken::verify(token, secret_);
    if (jwt) {
        revoked_tokens_[token] = jwt->expires_at;
    }
}

bool AuthManager::is_revoked(const std::string& token) {
    cleanup_expired_tokens();
    return revoked_tokens_.find(token) != revoked_tokens_.end();
}

std::optional<std::string> AuthManager::refresh_token(const std::string& old_token) {
    // 直接验证Token（不检查吊销状态）
    auto jwt = JWTToken::verify(old_token, secret_);
    if (!jwt) {
        return std::nullopt;
    }

    // 吊销旧Token
    revoked_tokens_[old_token] = jwt->expires_at;

    // 生成新Token（使用相同的TTL，但issued_at是当前时间）
    // 等待1秒确保时间戳不同（避免生成相同Token）
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto ttl = std::chrono::seconds(jwt->expires_at - jwt->issued_at);
    return generate_token(jwt->user_id, ttl);
}

void AuthManager::cleanup_expired_tokens() {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (auto it = revoked_tokens_.begin(); it != revoked_tokens_.end();) {
        if (it->second < now) {
            it = revoked_tokens_.erase(it);
        } else {
            ++it;
        }
    }
}

// JWTAuthenticator实现
JWTAuthenticator::JWTAuthenticator(const std::string& secret)
    : secret_(secret) {
}

AuthResult JWTAuthenticator::authenticate(const std::string& token) {
    auto jwt = JWTToken::verify(token, secret_);

    if (!jwt) {
        return AuthResult::fail("Invalid or expired token");
    }

    return AuthResult::ok(jwt->user_id);
}

}
