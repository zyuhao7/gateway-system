/**
 * @file test_metrics.cpp
 * @brief Metrics 单元测试
 */

#include "metrics.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        Metrics::instance().reset();
    }
};

// 测试连接计数
TEST_F(MetricsTest, ConnectionCounting) {
    auto& metrics = Metrics::instance();

    EXPECT_EQ(metrics.total_connections(), 0);
    EXPECT_EQ(metrics.active_connections(), 0);

    metrics.connection_opened();
    EXPECT_EQ(metrics.total_connections(), 1);

    metrics.connection_active();
    EXPECT_EQ(metrics.active_connections(), 1);

    metrics.connection_closed();
    EXPECT_EQ(metrics.active_connections(), 0);
}

// 测试消息计数
TEST_F(MetricsTest, MessageCounting) {
    auto& metrics = Metrics::instance();

    EXPECT_EQ(metrics.total_messages_received(), 0);
    EXPECT_EQ(metrics.total_messages_sent(), 0);

    metrics.message_received();
    metrics.message_received();
    EXPECT_EQ(metrics.total_messages_received(), 2);

    metrics.message_sent();
    EXPECT_EQ(metrics.total_messages_sent(), 1);
}

// 测试延迟记录
TEST_F(MetricsTest, LatencyRecording) {
    auto& metrics = Metrics::instance();

    metrics.record_latency(1000);  // 1ms
    metrics.record_latency(2000);  // 2ms
    metrics.record_latency(5000);  // 5ms

    // 验证 Prometheus 导出包含延迟数据
    std::string prometheus = metrics.export_prometheus();
    EXPECT_NE(prometheus.find("gateway_latency_microseconds"), std::string::npos);
}

// 测试线程安全
TEST_F(MetricsTest, ThreadSafety) {
    auto& metrics = Metrics::instance();

    const int num_threads = 10;
    const int ops_per_thread = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&metrics, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; j++) {
                metrics.connection_opened();
                metrics.message_received();
                metrics.message_sent();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(metrics.total_connections(), num_threads * ops_per_thread);
    EXPECT_EQ(metrics.total_messages_received(), num_threads * ops_per_thread);
    EXPECT_EQ(metrics.total_messages_sent(), num_threads * ops_per_thread);
}

// 测试 Prometheus 导出格式
TEST_F(MetricsTest, PrometheusExport) {
    auto& metrics = Metrics::instance();

    metrics.connection_opened();
    metrics.message_received();
    metrics.message_sent();

    std::string prometheus = metrics.export_prometheus();

    // 验证包含必要的指标
    EXPECT_NE(prometheus.find("gateway_connections_total"), std::string::npos);
    EXPECT_NE(prometheus.find("gateway_messages_received_total"), std::string::npos);
    EXPECT_NE(prometheus.find("gateway_messages_sent_total"), std::string::npos);

    // 验证格式正确（包含 HELP 和 TYPE）
    EXPECT_NE(prometheus.find("# HELP"), std::string::npos);
    EXPECT_NE(prometheus.find("# TYPE"), std::string::npos);
}

// 测试重置功能
TEST_F(MetricsTest, Reset) {
    auto& metrics = Metrics::instance();

    metrics.connection_opened();
    metrics.message_received();

    EXPECT_GT(metrics.total_connections(), 0);

    metrics.reset();

    EXPECT_EQ(metrics.total_connections(), 0);
    EXPECT_EQ(metrics.total_messages_received(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
