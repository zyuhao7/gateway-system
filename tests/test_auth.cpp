/**
 * @file test_auth.cpp
 * @brief 认证模块测试
 */

#include "auth.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace gateway;

void test_jwt_token_generation() {
    std::cout << "[Test 1] JWT Token generation..." << std::endl;

    AuthManager::instance().set_secret("test_secret_key_123");

    std::string token = AuthManager::instance().generate_token("user_alice");

    std::cout << "  Generated token: " << token << std::endl;
    assert(!token.empty());
    assert(token.find('.') != std::string::npos);

    std::cout << "✓ JWT Token generation passed" << std::endl;
}

void test_jwt_token_verification() {
    std::cout << "\n[Test 2] JWT Token verification..." << std::endl;

    AuthManager::instance().set_secret("test_secret_key_123");

    std::string token = AuthManager::instance().generate_token("user_bob");
    auto user_id = AuthManager::instance().verify_token(token);

    assert(user_id.has_value());
    assert(*user_id == "user_bob");

    std::cout << "  Verified user: " << *user_id << std::endl;
    std::cout << "✓ JWT Token verification passed" << std::endl;
}

void test_invalid_token() {
    std::cout << "\n[Test 3] Invalid token..." << std::endl;

    AuthManager::instance().set_secret("test_secret_key_123");

    // 无效Token
    auto result1 = AuthManager::instance().verify_token("invalid.token.here");
    assert(!result1.has_value());

    // 错误密钥
    std::string token = AuthManager::instance().generate_token("user_charlie");
    AuthManager::instance().set_secret("wrong_secret");
    auto result2 = AuthManager::instance().verify_token(token);
    assert(!result2.has_value());

    // 恢复正确密钥
    AuthManager::instance().set_secret("test_secret_key_123");

    std::cout << "✓ Invalid token handling passed" << std::endl;
}

void test_token_expiration() {
    std::cout << "\n[Test 4] Token expiration..." << std::endl;

    AuthManager::instance().set_secret("test_secret_key_123");

    // 生成1秒过期的Token
    std::string token = AuthManager::instance().generate_token(
        "user_dave",
        std::chrono::seconds(1)
    );

    // 立即验证应该成功
    auto result1 = AuthManager::instance().verify_token(token);
    assert(result1.has_value());
    std::cout << "  Token valid immediately" << std::endl;

    // 等待2秒后应该过期
    std::cout << "  Waiting 2 seconds for expiration..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto result2 = AuthManager::instance().verify_token(token);
    assert(!result2.has_value());
    std::cout << "  Token expired after 2 seconds" << std::endl;

    std::cout << "✓ Token expiration passed" << std::endl;
}

void test_token_revocation() {
    std::cout << "\n[Test 5] Token revocation..." << std::endl;

    AuthManager::instance().set_secret("test_secret_key_123");

    std::string token = AuthManager::instance().generate_token("user_eve");

    // 验证成功
    auto result1 = AuthManager::instance().verify_token(token);
    assert(result1.has_value());

    // 吊销Token
    AuthManager::instance().revoke_token(token);

    // 验证应该失败
    auto result2 = AuthManager::instance().verify_token(token);
    assert(!result2.has_value());

    std::cout << "✓ Token revocation passed" << std::endl;
}

void test_token_refresh() {
    std::cout << "\n[Test 6] Token refresh..." << std::endl;

    AuthManager::instance().set_secret("test_secret_key_123");

    std::string old_token = AuthManager::instance().generate_token("user_frank");

    // 刷新Token
    auto new_token_opt = AuthManager::instance().refresh_token(old_token);
    assert(new_token_opt.has_value());

    std::string new_token = *new_token_opt;

    // 旧Token应该失效
    auto result1 = AuthManager::instance().verify_token(old_token);
    assert(!result1.has_value());

    // 新Token应该有效
    auto result2 = AuthManager::instance().verify_token(new_token);
    assert(result2.has_value());
    assert(*result2 == "user_frank");

    std::cout << "✓ Token refresh passed" << std::endl;
}

void test_jwt_authenticator() {
    std::cout << "\n[Test 7] JWTAuthenticator..." << std::endl;

    JWTAuthenticator auth("test_secret_key_123");

    AuthManager::instance().set_secret("test_secret_key_123");
    std::string token = AuthManager::instance().generate_token("user_grace");

    // 认证成功
    auto result1 = auth.authenticate(token);
    assert(result1.success);
    assert(result1.user_id == "user_grace");
    std::cout << "  Authenticated user: " << result1.user_id << std::endl;

    // 认证失败
    auto result2 = auth.authenticate("invalid_token");
    assert(!result2.success);
    assert(!result2.error_message.empty());
    std::cout << "  Auth failed: " << result2.error_message << std::endl;

    std::cout << "✓ JWTAuthenticator passed" << std::endl;
}

int main() {
    std::cout << "=== Auth Module Test ===" << std::endl;
    std::cout << std::endl;

    test_jwt_token_generation();
    test_jwt_token_verification();
    test_invalid_token();
    test_token_expiration();
    test_token_revocation();
    test_token_refresh();
    test_jwt_authenticator();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
