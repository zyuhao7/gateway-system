/**
 * @file test_grpc_communication.cpp
 * @brief 测试 gRPC 跨节点通信
 */

#include "grpc_gateway_service.hpp"
#include "grpc_gateway_client.hpp"
#include "connection_manager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== gRPC Cross-Node Communication Test ===" << std::endl;

    // 启动 gRPC 服务器（模拟 node-2）
    std::cout << "\n[Test 1] Starting gRPC server on port 50051..." << std::endl;
    GrpcGatewayServer server(50051);
    server.start();
    std::cout << "✓ gRPC server started" << std::endl;

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 创建 gRPC 客户端（模拟 node-1）
    std::cout << "\n[Test 2] Creating gRPC client..." << std::endl;
    GrpcGatewayClient client;
    client.add_node("gateway-node-2", "localhost:50051");
    std::cout << "✓ gRPC client created" << std::endl;

    // 测试健康检查
    std::cout << "\n[Test 3] Testing health check..." << std::endl;
    bool healthy = client.health_check("gateway-node-2");
    if (healthy) {
        std::cout << "✓ Node is healthy" << std::endl;
    } else {
        std::cout << "✗ Health check failed" << std::endl;
    }

    // 测试获取节点统计
    std::cout << "\n[Test 4] Getting node stats..." << std::endl;
    std::string stats = client.get_node_stats("gateway-node-2");
    std::cout << "Node stats: " << stats << std::endl;

    // 测试消息转发
    std::cout << "\n[Test 5] Testing message forwarding..." << std::endl;
    bool success = client.forward_message(
        "gateway-node-2",
        "user_12345",
        "Hello from node-1!",
        "gateway-node-1"
    );
    if (success) {
        std::cout << "✓ Message forwarded successfully" << std::endl;
    } else {
        std::cout << "✗ Message forwarding failed" << std::endl;
    }

    // 测试批量转发
    std::cout << "\n[Test 6] Testing batch message forwarding..." << std::endl;
    std::vector<std::tuple<std::string, std::string>> messages = {
        {"user_001", "Message 1"},
        {"user_002", "Message 2"},
        {"user_003", "Message 3"}
    };
    int count = client.forward_message_batch("gateway-node-2", messages, "gateway-node-1");
    std::cout << "Forwarded " << count << " messages" << std::endl;

    // 停止服务器
    std::cout << "\n[Test 7] Stopping gRPC server..." << std::endl;
    server.stop();
    std::cout << "✓ gRPC server stopped" << std::endl;

    std::cout << "\n=== All Tests Completed ===" << std::endl;
    return 0;
}
