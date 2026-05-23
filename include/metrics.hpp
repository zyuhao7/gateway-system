/**
 * @file metrics.hpp
 * @brief Prometheus 指标采集
 *
 * 收集网关系统的关键性能指标：
 * - 连接数（总数、活跃数、空闲数）
 * - 消息吞吐（QPS）
 * - 延迟分布（P50/P99/P999）
 * - 错误率
 */

#pragma once

#include <string>
#include <atomic>
#include <chrono>
#include <map>
#include <shared_mutex>

/**
 * @class Metrics
 * @brief 指标收集器（单例模式）
 *
 * 提供轻量级的指标收集，可导出为 Prometheus 格式。
 * 线程安全，适合高并发场景。
 */
class Metrics {
public:
    static Metrics& instance();

    // 连接指标
    void connection_opened();
    void connection_closed();
    void connection_active();
    void connection_idle();

    // 消息指标
    void message_received();
    void message_sent();
    void message_error();

    // 延迟指标（微秒）
    void record_latency(uint64_t latency_us);

    // 获取指标
    uint64_t total_connections() const;
    uint64_t active_connections() const;
    uint64_t total_messages_received() const;
    uint64_t total_messages_sent() const;
    uint64_t total_errors() const;

    // 导出 Prometheus 格式
    std::string export_prometheus() const;

    // 重置计数器（用于测试）
    void reset();

private:
    Metrics() = default;

    // 连接指标
    std::atomic<uint64_t> total_connections_{0};
    std::atomic<uint64_t> active_connections_{0};
    std::atomic<uint64_t> closed_connections_{0};

    // 消息指标
    std::atomic<uint64_t> messages_received_{0};
    std::atomic<uint64_t> messages_sent_{0};
    std::atomic<uint64_t> errors_{0};

    // 延迟统计（简化版，实际应使用 histogram）
    mutable std::shared_mutex latency_mutex_;
    std::vector<uint64_t> latency_samples_;
    static constexpr size_t MAX_SAMPLES = 10000;
};
