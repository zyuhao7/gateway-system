/**
 * @file metrics.cpp
 * @brief Prometheus 指标实现
 */

#include "metrics.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <mutex>

Metrics& Metrics::instance() {
    static Metrics metrics;
    return metrics;
}

void Metrics::connection_opened() {
    total_connections_.fetch_add(1, std::memory_order_relaxed);
    active_connections_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::connection_closed() {
    active_connections_.fetch_sub(1, std::memory_order_relaxed);
    closed_connections_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::connection_active() {
    // 标记连接为活跃状态（可扩展）
}

void Metrics::connection_idle() {
    // 标记连接为空闲状态（可扩展）
}

void Metrics::message_received() {
    messages_received_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::message_sent() {
    messages_sent_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::message_error() {
    errors_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::record_latency(uint64_t latency_us) {
    std::unique_lock lock(latency_mutex_);

    // 保留最近的 N 个样本
    if (latency_samples_.size() >= MAX_SAMPLES) {
        latency_samples_.erase(latency_samples_.begin());
    }

    latency_samples_.push_back(latency_us);
}

uint64_t Metrics::total_connections() const {
    return total_connections_.load(std::memory_order_relaxed);
}

uint64_t Metrics::active_connections() const {
    return active_connections_.load(std::memory_order_relaxed);
}

uint64_t Metrics::total_messages_received() const {
    return messages_received_.load(std::memory_order_relaxed);
}

uint64_t Metrics::total_messages_sent() const {
    return messages_sent_.load(std::memory_order_relaxed);
}

uint64_t Metrics::total_errors() const {
    return errors_.load(std::memory_order_relaxed);
}

/**
 * 导出 Prometheus 格式的指标
 *
 * 格式：
 * # HELP metric_name Description
 * # TYPE metric_name type
 * metric_name{label="value"} value
 */
std::string Metrics::export_prometheus() const {
    std::ostringstream oss;

    // 连接指标
    oss << "# HELP gateway_connections_total Total number of connections\n";
    oss << "# TYPE gateway_connections_total counter\n";
    oss << "gateway_connections_total " << total_connections() << "\n\n";

    oss << "# HELP gateway_connections_active Active connections\n";
    oss << "# TYPE gateway_connections_active gauge\n";
    oss << "gateway_connections_active " << active_connections() << "\n\n";

    oss << "# HELP gateway_connections_closed Closed connections\n";
    oss << "# TYPE gateway_connections_closed counter\n";
    oss << "gateway_connections_closed " << closed_connections_.load() << "\n\n";

    // 消息指标
    oss << "# HELP gateway_messages_received_total Total messages received\n";
    oss << "# TYPE gateway_messages_received_total counter\n";
    oss << "gateway_messages_received_total " << total_messages_received() << "\n\n";

    oss << "# HELP gateway_messages_sent_total Total messages sent\n";
    oss << "# TYPE gateway_messages_sent_total counter\n";
    oss << "gateway_messages_sent_total " << total_messages_sent() << "\n\n";

    oss << "# HELP gateway_errors_total Total errors\n";
    oss << "# TYPE gateway_errors_total counter\n";
    oss << "gateway_errors_total " << total_errors() << "\n\n";

    // 延迟统计
    std::shared_lock lock(latency_mutex_);
    if (!latency_samples_.empty()) {
        auto samples = latency_samples_;
        lock.unlock();

        std::sort(samples.begin(), samples.end());

        auto p50 = samples[samples.size() * 50 / 100];
        auto p99 = samples[samples.size() * 99 / 100];
        auto p999 = samples[samples.size() * 999 / 1000];

        oss << "# HELP gateway_latency_microseconds Message latency\n";
        oss << "# TYPE gateway_latency_microseconds summary\n";
        oss << "gateway_latency_microseconds{quantile=\"0.5\"} " << p50 << "\n";
        oss << "gateway_latency_microseconds{quantile=\"0.99\"} " << p99 << "\n";
        oss << "gateway_latency_microseconds{quantile=\"0.999\"} " << p999 << "\n\n";
    }

    // 缓冲池指标
    oss << "# HELP gateway_buffer_pool_allocated Total allocated buffers\n";
    oss << "# TYPE gateway_buffer_pool_allocated gauge\n";
    oss << "gateway_buffer_pool_allocated " << buffer_pool_allocated_.load() << "\n\n";

    oss << "# HELP gateway_buffer_pool_in_use Buffers currently in use\n";
    oss << "# TYPE gateway_buffer_pool_in_use gauge\n";
    oss << "gateway_buffer_pool_in_use " << buffer_pool_in_use_.load() << "\n\n";

    oss << "# HELP gateway_buffer_pool_available Available buffers in pool\n";
    oss << "# TYPE gateway_buffer_pool_available gauge\n";
    oss << "gateway_buffer_pool_available " << buffer_pool_available_.load() << "\n\n";

    return oss.str();
}

void Metrics::reset() {
    total_connections_.store(0);
    active_connections_.store(0);
    closed_connections_.store(0);
    messages_received_.store(0);
    messages_sent_.store(0);
    errors_.store(0);

    std::unique_lock lock(latency_mutex_);
    latency_samples_.clear();
}

void Metrics::record_buffer_pool_stats(size_t allocated, size_t in_use, size_t available) {
    buffer_pool_allocated_.store(allocated, std::memory_order_relaxed);
    buffer_pool_in_use_.store(in_use, std::memory_order_relaxed);
    buffer_pool_available_.store(available, std::memory_order_relaxed);
}
