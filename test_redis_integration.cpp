/**
 * @file test_redis_integration.cpp
 * @brief 测试 Redis 集成功能
 */

#include "node_registry.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Redis Integration Test ===" << std::endl;

    // 创建 NodeRegistry 实例
    NodeRegistry registry("localhost", 6379);

    // 设置节点变化回调
    registry.set_node_change_callback([](const std::string& node_id, bool added) {
        if (added) {
            std::cout << "Callback: Node added - " << node_id << std::endl;
        } else {
            std::cout << "Callback: Node removed - " << node_id << std::endl;
        }
    });

    // 注册节点 1
    std::cout << "\n[Test 1] Registering node-1..." << std::endl;
    if (registry.register_node("gateway-node-1", "192.168.1.10", 8080)) {
        std::cout << "✓ Node-1 registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register node-1" << std::endl;
        return 1;
    }

    // 启动后台任务
    std::cout << "\n[Test 2] Starting background tasks..." << std::endl;
    registry.start();
    std::cout << "✓ Background tasks started" << std::endl;

    // 等待心跳发送
    std::cout << "\n[Test 3] Waiting for heartbeat (6 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(6));
    std::cout << "✓ Heartbeat sent" << std::endl;

    // 在另一个进程中注册节点 2（模拟）
    std::cout << "\n[Test 4] Simulating node-2 registration in Redis..." << std::endl;
    system("redis-cli HSET gateway:nodes gateway-node-2 '192.168.1.11:8081' > /dev/null");
    system("redis-cli SETEX gateway:heartbeat:gateway-node-2 15 alive > /dev/null");
    std::cout << "✓ Node-2 added to Redis" << std::endl;

    // 等待发现新节点
    std::cout << "\n[Test 5] Waiting for node discovery (6 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // 检查存活节点
    std::cout << "\n[Test 6] Checking alive nodes..." << std::endl;
    auto alive_nodes = registry.get_alive_nodes();
    std::cout << "Alive nodes count: " << alive_nodes.size() << std::endl;
    for (const auto& node_id : alive_nodes) {
        auto info = registry.get_node_info(node_id);
        std::cout << "  - " << node_id << " (" << info.host << ":" << info.port << ")" << std::endl;
    }

    // 测试一致性哈希
    std::cout << "\n[Test 7] Testing consistent hashing..." << std::endl;
    std::string user1 = "user_12345";
    std::string user2 = "user_67890";
    std::string node1 = registry.get_node_for_key(user1);
    std::string node2 = registry.get_node_for_key(user2);
    std::cout << "  " << user1 << " -> " << node1 << std::endl;
    std::cout << "  " << user2 << " -> " << node2 << std::endl;

    // 测试节点超时
    std::cout << "\n[Test 8] Testing node timeout (waiting 20 seconds)..." << std::endl;
    std::cout << "  Node-2 will timeout after 15 seconds without heartbeat" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(20));

    alive_nodes = registry.get_alive_nodes();
    std::cout << "Alive nodes after timeout: " << alive_nodes.size() << std::endl;
    for (const auto& node_id : alive_nodes) {
        std::cout << "  - " << node_id << std::endl;
    }

    // 停止
    std::cout << "\n[Test 9] Stopping NodeRegistry..." << std::endl;
    registry.stop();
    std::cout << "✓ NodeRegistry stopped" << std::endl;

    std::cout << "\n=== All Tests Completed ===" << std::endl;
    return 0;
}
