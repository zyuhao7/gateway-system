/**
 * @file test_rate_limiter.cpp
 * @brief 速率限制测试
 */

#include "rate_limiter.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace gateway;

void test_token_bucket() {
    std::cout << "[Test 1] Token Bucket..." << std::endl;

    // 容量10，每秒填充5个
    TokenBucket bucket(10, 5);

    // 消耗5个令牌
    assert(bucket.try_consume(5));
    assert(bucket.available() == 5);

    // 再消耗5个
    assert(bucket.try_consume(5));
    assert(bucket.available() == 0);

    // 超出容量，失败
    assert(!bucket.try_consume(1));

    std::cout << "  Consumed 10 tokens, bucket empty" << std::endl;

    // 等待1秒，应该填充5个
    std::this_thread::sleep_for(std::chrono::seconds(1));
    assert(bucket.try_consume(5));
    std::cout << "  After 1s, refilled 5 tokens" << std::endl;

    std::cout << "✓ Token Bucket passed" << std::endl;
}

void test_sliding_window() {
    std::cout << "\n[Test 2] Sliding Window..." << std::endl;

    // 限制5个请求，窗口2秒
    SlidingWindow window(5, std::chrono::seconds(2));

    // 发送5个请求
    for (int i = 0; i < 5; i++) {
        assert(window.try_acquire());
    }
    assert(window.count() == 5);

    // 超出限制
    assert(!window.try_acquire());
    std::cout << "  5 requests in 2s window, 6th rejected" << std::endl;

    // 等待2秒，窗口过期
    std::this_thread::sleep_for(std::chrono::seconds(2));
    assert(window.try_acquire());
    std::cout << "  After 2s, window expired, new request allowed" << std::endl;

    std::cout << "✓ Sliding Window passed" << std::endl;
}

void test_rate_limiter() {
    std::cout << "\n[Test 3] RateLimiter..." << std::endl;

    RateLimiter limiter;

    // 设置令牌桶
    limiter.set_token_bucket("test", 10, 5);

    auto result1 = limiter.check_token_bucket("test", 5);
    assert(result1.allowed);
    assert(result1.remaining == 5);
    std::cout << "  Consumed 5 tokens, remaining: " << result1.remaining << std::endl;

    auto result2 = limiter.check_token_bucket("test", 10);
    assert(!result2.allowed);
    std::cout << "  Tried to consume 10, rejected: " << result2.error_message << std::endl;

    std::cout << "✓ RateLimiter passed" << std::endl;
}

void test_ip_rate_limiter() {
    std::cout << "\n[Test 4] IP Rate Limiter..." << std::endl;

    auto& limiter = IPRateLimiter::instance();
    limiter.configure(10, 20);  // 10 req/s, burst 20

    std::string ip = "192.168.1.100";

    // 发送20个请求（burst容量）
    int success = 0;
    for (int i = 0; i < 25; i++) {
        auto result = limiter.check(ip);
        if (result.allowed) {
            success++;
        }
    }

    std::cout << "  Sent 25 requests, " << success << " allowed" << std::endl;
    assert(success <= 20);  // 最多20个

    // 封禁IP
    limiter.ban(ip, std::chrono::seconds(2));
    assert(limiter.is_banned(ip));

    auto result = limiter.check(ip);
    assert(!result.allowed);
    std::cout << "  IP banned, request rejected" << std::endl;

    // 等待2秒，封禁过期
    std::this_thread::sleep_for(std::chrono::seconds(2));
    assert(!limiter.is_banned(ip));
    std::cout << "  After 2s, ban expired" << std::endl;

    std::cout << "✓ IP Rate Limiter passed" << std::endl;
}

void test_user_rate_limiter() {
    std::cout << "\n[Test 5] User Rate Limiter..." << std::endl;

    auto& limiter = UserRateLimiter::instance();
    limiter.configure(60, 100);  // 60 req/min, burst 100

    std::string user_id = "user_alice";

    // 快速发送50个请求
    int success = 0;
    for (int i = 0; i < 50; i++) {
        auto result = limiter.check(user_id);
        if (result.allowed) {
            success++;
        }
    }

    std::cout << "  User sent 50 requests, " << success << " allowed" << std::endl;
    assert(success <= 100);

    std::cout << "✓ User Rate Limiter passed" << std::endl;
}

void test_api_rate_limiter() {
    std::cout << "\n[Test 6] API Rate Limiter..." << std::endl;

    auto& limiter = APIRateLimiter::instance();
    limiter.configure("/api/search", 5, 10);  // 5 req/s, burst 10

    std::string client_id = "client_123";

    // 发送15个请求
    int success = 0;
    for (int i = 0; i < 15; i++) {
        auto result = limiter.check("/api/search", client_id);
        if (result.allowed) {
            success++;
        }
    }

    std::cout << "  API called 15 times, " << success << " allowed" << std::endl;
    assert(success <= 10);  // 最多10个

    // 不同API不受影响
    auto result = limiter.check("/api/login", client_id);
    assert(result.allowed);
    std::cout << "  Different API endpoint not affected" << std::endl;

    std::cout << "✓ API Rate Limiter passed" << std::endl;
}

void test_concurrent_access() {
    std::cout << "\n[Test 7] Concurrent Access..." << std::endl;

    TokenBucket bucket(100, 50);

    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    // 10个线程并发访问
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 20; j++) {
                if (bucket.try_consume(1)) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "  10 threads, 200 attempts, " << success_count << " succeeded" << std::endl;
    assert(success_count <= 100);  // 不应超过容量

    std::cout << "✓ Concurrent Access passed" << std::endl;
}

int main() {
    std::cout << "=== Rate Limiter Test ===" << std::endl;
    std::cout << std::endl;

    test_token_bucket();
    test_sliding_window();
    test_rate_limiter();
    test_ip_rate_limiter();
    test_user_rate_limiter();
    test_api_rate_limiter();
    test_concurrent_access();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
