/**
 * @file rate_limiter.cpp
 * @brief 速率限制实现
 */

#include "rate_limiter.hpp"
#include <algorithm>

namespace gateway {

// TokenBucket 实现
TokenBucket::TokenBucket(int capacity, int refill_rate)
    : capacity_(capacity)
    , refill_rate_(refill_rate)
    , tokens_(capacity)
    , last_refill_(std::chrono::steady_clock::now()) {
}

void TokenBucket::refill() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_refill_
    ).count() / 1000.0;

    double new_tokens = elapsed * refill_rate_;
    tokens_ = std::min(tokens_ + new_tokens, static_cast<double>(capacity_));
    last_refill_ = now;
}

bool TokenBucket::try_consume(int tokens) {
    std::lock_guard<std::mutex> lock(mutex_);
    refill();

    if (tokens_ >= tokens) {
        tokens_ -= tokens;
        return true;
    }
    return false;
}

int TokenBucket::available() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(tokens_);
}

void TokenBucket::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    tokens_ = capacity_;
    last_refill_ = std::chrono::steady_clock::now();
}

// SlidingWindow 实现
SlidingWindow::SlidingWindow(int limit, std::chrono::seconds window)
    : limit_(limit)
    , window_(window) {
}

void SlidingWindow::cleanup() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - window_;

    while (!requests_.empty() && requests_.front() < cutoff) {
        requests_.pop_front();
    }
}

bool SlidingWindow::try_acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    cleanup();

    if (requests_.size() < static_cast<size_t>(limit_)) {
        requests_.push_back(std::chrono::steady_clock::now());
        return true;
    }
    return false;
}

int SlidingWindow::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requests_.size();
}

void SlidingWindow::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    requests_.clear();
}

// RateLimiter 实现
RateLimiter::RateLimiter() {
}

void RateLimiter::set_token_bucket(const std::string& key,
                                   int capacity,
                                   int refill_rate) {
    std::lock_guard<std::mutex> lock(mutex_);
    token_buckets_[key] = std::make_shared<TokenBucket>(capacity, refill_rate);
    last_access_[key] = std::chrono::steady_clock::now();
}

void RateLimiter::set_sliding_window(const std::string& key,
                                     int limit,
                                     std::chrono::seconds window) {
    std::lock_guard<std::mutex> lock(mutex_);
    sliding_windows_[key] = std::make_shared<SlidingWindow>(limit, window);
    last_access_[key] = std::chrono::steady_clock::now();
}

RateLimitResult RateLimiter::check_token_bucket(const std::string& key, int tokens) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = token_buckets_.find(key);
    if (it == token_buckets_.end()) {
        // 未配置，默认拒绝（需要先配置）
        return RateLimitResult::reject("Rate limiter not configured", 0);
    }

    last_access_[key] = std::chrono::steady_clock::now();

    if (it->second->try_consume(tokens)) {
        int remaining = it->second->available();
        return RateLimitResult::ok(remaining, 0);
    } else {
        auto reset_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count() + 1;
        return RateLimitResult::reject("Rate limit exceeded", reset_time);
    }
}

RateLimitResult RateLimiter::check_sliding_window(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sliding_windows_.find(key);
    if (it == sliding_windows_.end()) {
        // 未配置，默认允许
        return RateLimitResult::ok(999, 0);
    }

    last_access_[key] = std::chrono::steady_clock::now();

    if (it->second->try_acquire()) {
        int remaining = it->second->count();
        return RateLimitResult::ok(remaining, 0);
    } else {
        auto reset_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count() + 60;
        return RateLimitResult::reject("Rate limit exceeded", reset_time);
    }
}

void RateLimiter::reset(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto tb_it = token_buckets_.find(key);
    if (tb_it != token_buckets_.end()) {
        tb_it->second->reset();
    }

    auto sw_it = sliding_windows_.find(key);
    if (sw_it != sliding_windows_.end()) {
        sw_it->second->reset();
    }
}

void RateLimiter::cleanup_expired(std::chrono::seconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - ttl;

    for (auto it = last_access_.begin(); it != last_access_.end();) {
        if (it->second < cutoff) {
            token_buckets_.erase(it->first);
            sliding_windows_.erase(it->first);
            it = last_access_.erase(it);
        } else {
            ++it;
        }
    }
}

// IPRateLimiter 实现
IPRateLimiter& IPRateLimiter::instance() {
    static IPRateLimiter limiter;
    return limiter;
}

void IPRateLimiter::configure(int requests_per_second, int burst_capacity) {
    requests_per_second_ = requests_per_second;
    burst_capacity_ = burst_capacity;
}

RateLimitResult IPRateLimiter::check(const std::string& ip) {
    // 检查是否被封禁
    if (is_banned(ip)) {
        return RateLimitResult::reject("IP banned", 0);
    }

    std::string key = "ip:" + ip;

    // 检查限流器是否已存在，不存在则创建
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 只在不存在时创建
        auto result = limiter_.check_token_bucket(key, 0);  // 先检查不消耗
        if (result.error_message == "Rate limiter not configured") {
            limiter_.set_token_bucket(key, burst_capacity_, requests_per_second_);
        }
    }

    return limiter_.check_token_bucket(key, 1);
}

void IPRateLimiter::ban(const std::string& ip, std::chrono::seconds duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto unban_time = std::chrono::steady_clock::now() + duration;
    banned_ips_[ip] = unban_time;
}

bool IPRateLimiter::is_banned(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = banned_ips_.find(ip);
    if (it == banned_ips_.end()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    if (now >= it->second) {
        // 封禁已过期
        banned_ips_.erase(it);
        return false;
    }

    return true;
}

// UserRateLimiter 实现
UserRateLimiter& UserRateLimiter::instance() {
    static UserRateLimiter limiter;
    return limiter;
}

void UserRateLimiter::configure(int requests_per_minute, int burst_capacity) {
    requests_per_minute_ = requests_per_minute;
    burst_capacity_ = burst_capacity;
}

RateLimitResult UserRateLimiter::check(const std::string& user_id) {
    std::string key = "user:" + user_id;

    // 检查限流器是否已存在，不存在则创建
    auto result = limiter_.check_token_bucket(key, 0);
    if (result.error_message == "Rate limiter not configured") {
        int refill_rate = requests_per_minute_ / 60;  // 转换为每秒
        limiter_.set_token_bucket(key, burst_capacity_, refill_rate);
    }

    return limiter_.check_token_bucket(key, 1);
}

// APIRateLimiter 实现
APIRateLimiter& APIRateLimiter::instance() {
    static APIRateLimiter limiter;
    return limiter;
}

void APIRateLimiter::configure(const std::string& api_path,
                               int requests_per_second,
                               int burst_capacity) {
    std::lock_guard<std::mutex> lock(mutex_);
    api_configs_[api_path] = {requests_per_second, burst_capacity};
}

RateLimitResult APIRateLimiter::check(const std::string& api_path,
                                      const std::string& client_id) {
    std::pair<int, int> config;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = api_configs_.find(api_path);
        if (it == api_configs_.end()) {
            // 未配置，默认允许
            return RateLimitResult::ok(999, 0);
        }
        config = it->second;
    }

    std::string key = "api:" + api_path + ":" + client_id;
    auto [rps, burst] = config;

    // 检查限流器是否已存在，不存在则创建
    auto result = limiter_.check_token_bucket(key, 0);
    if (result.error_message == "Rate limiter not configured") {
        limiter_.set_token_bucket(key, burst, rps);
    }

    return limiter_.check_token_bucket(key, 1);
}

}
