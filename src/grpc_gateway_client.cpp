/**
 * @file grpc_gateway_client.cpp
 * @brief gRPC 网关客户端实现
 */

#include "grpc_gateway_client.hpp"
#include <iostream>
#include <chrono>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

void GrpcGatewayClient::add_node(const std::string& node_id, const std::string& address) {
    std::unique_lock lock(mutex_);
    node_addresses_[node_id] = address;
    stubs_.erase(node_id);  // 清除旧的 stub，下次使用时重新创建
    std::cout << "Added remote node: " << node_id << " at " << address << std::endl;
}

void GrpcGatewayClient::remove_node(const std::string& node_id) {
    std::unique_lock lock(mutex_);
    node_addresses_.erase(node_id);
    stubs_.erase(node_id);
    std::cout << "Removed remote node: " << node_id << std::endl;
}

std::shared_ptr<gateway::GatewayService::Stub> GrpcGatewayClient::get_stub(const std::string& node_id) {
    std::shared_lock read_lock(mutex_);

    // 检查是否已有 stub
    auto stub_it = stubs_.find(node_id);
    if (stub_it != stubs_.end()) {
        return stub_it->second;
    }

    // 获取地址
    auto addr_it = node_addresses_.find(node_id);
    if (addr_it == node_addresses_.end()) {
        std::cerr << "Node not found: " << node_id << std::endl;
        return nullptr;
    }

    std::string address = addr_it->second;
    read_lock.unlock();

    // 创建新的 stub
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = std::shared_ptr<gateway::GatewayService::Stub>(
        gateway::GatewayService::NewStub(channel)
    );

    std::unique_lock write_lock(mutex_);
    stubs_[node_id] = stub;
    std::cout << "Created gRPC stub for node: " << node_id << " at " << address << std::endl;

    return stub;
}

bool GrpcGatewayClient::forward_message(const std::string& node_id,
                                       const std::string& user_id,
                                       const std::string& message,
                                       const std::string& from_node) {
    auto stub = get_stub(node_id);
    if (!stub) {
        return false;
    }

    gateway::ForwardMessageRequest request;
    request.set_user_id(user_id);
    request.set_message(message);
    request.set_from_node(from_node);
    request.set_timestamp(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    gateway::ForwardMessageResponse response;
    ClientContext context;

    // 设置超时
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

    Status status = stub->ForwardMessage(&context, request, &response);

    if (status.ok()) {
        if (response.success()) {
            std::cout << "Message forwarded to " << node_id << " for user " << user_id << std::endl;
            return true;
        } else {
            std::cerr << "Forward failed: " << response.error_message() << std::endl;
            return false;
        }
    } else {
        std::cerr << "gRPC call failed: " << status.error_code() << ": "
                  << status.error_message() << std::endl;
        return false;
    }
}

int GrpcGatewayClient::forward_message_batch(
    const std::string& node_id,
    const std::vector<std::tuple<std::string, std::string>>& messages,
    const std::string& from_node) {

    auto stub = get_stub(node_id);
    if (!stub) {
        return 0;
    }

    gateway::ForwardMessageBatchRequest request;
    int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (const auto& [user_id, message] : messages) {
        auto* msg = request.add_messages();
        msg->set_user_id(user_id);
        msg->set_message(message);
        msg->set_from_node(from_node);
        msg->set_timestamp(timestamp);
    }

    gateway::ForwardMessageBatchResponse response;
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));

    Status status = stub->ForwardMessageBatch(&context, request, &response);

    if (status.ok()) {
        std::cout << "Batch forward to " << node_id << ": "
                  << response.success_count() << " success, "
                  << response.failed_count() << " failed" << std::endl;
        return response.success_count();
    } else {
        std::cerr << "gRPC batch call failed: " << status.error_code() << ": "
                  << status.error_message() << std::endl;
        return 0;
    }
}

bool GrpcGatewayClient::health_check(const std::string& node_id) {
    auto stub = get_stub(node_id);
    if (!stub) {
        return false;
    }

    gateway::HealthCheckRequest request;
    request.set_node_id(node_id);

    gateway::HealthCheckResponse response;
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));

    Status status = stub->HealthCheck(&context, request, &response);

    if (status.ok() && response.healthy()) {
        std::cout << "Node " << node_id << " is healthy: "
                  << response.active_connections() << " active connections" << std::endl;
        return true;
    }

    return false;
}

std::string GrpcGatewayClient::get_node_stats(const std::string& node_id) {
    auto stub = get_stub(node_id);
    if (!stub) {
        return "{}";
    }

    gateway::NodeStatsRequest request;
    request.set_node_id(node_id);

    gateway::NodeStatsResponse response;
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));

    Status status = stub->GetNodeStats(&context, request, &response);

    if (status.ok()) {
        // 简单的 JSON 格式化
        std::ostringstream json;
        json << "{"
             << "\"node_id\":\"" << response.node_id() << "\","
             << "\"active_connections\":" << response.active_connections() << ","
             << "\"total_connections\":" << response.total_connections() << ","
             << "\"messages_received\":" << response.messages_received() << ","
             << "\"messages_sent\":" << response.messages_sent() << ","
             << "\"uptime_seconds\":" << response.uptime_seconds()
             << "}";
        return json.str();
    }

    return "{}";
}
