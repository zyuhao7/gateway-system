/**
 * @file rate_limiter.hpp
 * @brief 速率限制模块
 */

#pragma once

#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <deque>
#include <memory>

namespace gateway {

// 速率限制结果
struct RateLimitResult {
    bool allowed;           // 是否允许
    int remaining;          // 剩余配额
    int64_t reset_time;     // 重置时间（秒）
    std::string error_message;

    static RateLimitResult ok(int remaining, int64_t reset_time) {
        return {true, remaining, reset_time, ""};
    }

    static RateLimitResult reject(const std::string& error, int64_t reset_time) {
        return {false, 0, reset_time, error};
    }
};

// 令牌桶算法
class TokenBucket {
public:
    TokenBucket(int capacity, int refill_rate);

    // 尝试获取N个令牌
    bool try_consume(int tokens = 1);

    // 获取剩余令牌数
    int available() const;

    // 重置
    void reset();

private:
    int capacity_;              // 桶容量
    int refill_rate_;          // 填充速率（令牌/秒）
    double tokens_;             // 当前令牌数
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;

    void refill();
};

// 滑动窗口算法
class SlidingWindow {
public:
    SlidingWindow(int limit, std::chrono::seconds window);

    // 记录一次请求
    bool try_acquire();

    // 获取当前窗口内的请求数
    int count() const;

    // 重置
    void reset();

private:
    int limit_;                 // 限制数量
    std::chrono::seconds window_;   // 时间窗口
    std::deque<std::chrono::steady_clock::time_point> requests_;
    mutable std::mutex mutex_;

    void cleanup();
};

// 速率限制器（组合多种算法）
class RateLimiter {
public:
    // 令牌桶配置
    struct TokenBucketConfig {
        int capacity;
        int refill_rate;
    };

    // 滑动窗口配置
    struct SlidingWindowConfig {
        int limit;
        int window_seconds;
    };

    RateLimiter();

    // 设置令牌桶限制
    void set_token_bucket(const std::string& key,
                         int capacity,
                         int refill_rate);

    // 设置滑动窗口限制
    void set_sliding_window(const std::string& key,
                           int limit,
                           std::chrono::seconds window);

    // 检查是否允许（令牌桶）
    RateLimitResult check_token_bucket(const std::string& key, int tokens = 1);

    // 检查是否允许（滑动窗口）
    RateLimitResult check_sliding_window(const std::string& key);

    // 重置某个key的限制
    void reset(const std::string& key);

    // 清理过期数据
    void cleanup_expired(std::chrono::seconds ttl);

private:
    std::map<std::string, std::shared_ptr<TokenBucket>> token_buckets_;
    std::map<std::string, std::shared_ptr<SlidingWindow>> sliding_windows_;
    std::map<std::string, std::chrono::steady_clock::time_point> last_access_;
    mutable std::mutex mutex_;
};

// IP速率限制器
class IPRateLimiter {
public:
    static IPRateLimiter& instance();

    // 配置IP限制
    void configure(int requests_per_second, int burst_capacity);

    // 检查IP是否允许
    RateLimitResult check(const std::string& ip);

    // 封禁IP
    void ban(const std::string& ip, std::chrono::seconds duration);

    // 检查IP是否被封禁
    bool is_banned(const std::string& ip);

private:
    IPRateLimiter() = default;

    RateLimiter limiter_;
    std::map<std::string, std::chrono::steady_clock::time_point> banned_ips_;
    std::mutex mutex_;

    int requests_per_second_ = 100;
    int burst_capacity_ = 200;
};

// 用户速率限制器
class UserRateLimiter {
public:
    static UserRateLimiter& instance();

    // 配置用户限制
    void configure(int requests_per_minute, int burst_capacity);

    // 检查用户是否允许
    RateLimitResult check(const std::string& user_id);

private:
    UserRateLimiter() = default;

    RateLimiter limiter_;
    int requests_per_minute_ = 1000;
    int burst_capacity_ = 1500;
};

// API端点速率限制器
class APIRateLimiter {
public:
    static APIRateLimiter& instance();

    // 为特定API设置限制
    void configure(const std::string& api_path,
                  int requests_per_second,
                  int burst_capacity);

    // 检查API调用是否允许
    RateLimitResult check(const std::string& api_path,
                         const std::string& client_id);

private:
    APIRateLimiter() = default;

    std::map<std::string, std::pair<int, int>> api_configs_;  // path -> (rps, burst)
    RateLimiter limiter_;
    std::mutex mutex_;
};

}
