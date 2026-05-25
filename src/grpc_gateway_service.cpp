/**
 * @file grpc_gateway_service.cpp
 * @brief gRPC 网关服务实现
 */

#include "grpc_gateway_service.hpp"
#include "connection_manager.hpp"
#include "metrics.hpp"
#include <iostream>
#include <chrono>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// ============================================================================
// GrpcGatewayService Implementation
// ============================================================================

GrpcGatewayService::GrpcGatewayService()
    : start_time_(std::chrono::steady_clock::now()) {
}

Status GrpcGatewayService::ForwardMessage(
    ServerContext* /*context*/,
    const gateway::ForwardMessageRequest* request,
    gateway::ForwardMessageResponse* response) {

    std::cout << "Received ForwardMessage from " << request->from_node()
              << " for user " << request->user_id() << std::endl;

    // TODO: 根据 user_id 查找对应的连接
    // 当前简化实现：假设 user_id 就是 connection_id
    try {
        uint64_t conn_id = std::stoull(request->user_id());
        auto conn = gateway::ConnectionManager::instance().find(conn_id);

        if (conn) {
            // 发送消息到用户连接
            // TODO: 需要在 Connection 类中添加异步发送消息的方法
            // conn->send_message(request->message());

            response->set_success(true);
            response->set_user_online(true);
            std::cout << "Message forwarded to user " << request->user_id() << std::endl;
        } else {
            response->set_success(false);
            response->set_user_online(false);
            response->set_error_message("User not found or offline");
            std::cout << "User " << request->user_id() << " not found" << std::endl;
        }
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_user_online(false);
        response->set_error_message(std::string("Invalid user_id: ") + e.what());
    }

    return Status::OK;
}

Status GrpcGatewayService::ForwardMessageBatch(
    ServerContext* context,
    const gateway::ForwardMessageBatchRequest* request,
    gateway::ForwardMessageBatchResponse* response) {

    int success_count = 0;
    int failed_count = 0;

    for (const auto& msg : request->messages()) {
        gateway::ForwardMessageResponse single_response;
        Status status = ForwardMessage(context, &msg, &single_response);

        if (status.ok() && single_response.success()) {
            success_count++;
        } else {
            failed_count++;
            response->add_errors(single_response.error_message());
        }
    }

    response->set_success_count(success_count);
    response->set_failed_count(failed_count);

    std::cout << "Batch forward completed: " << success_count << " success, "
              << failed_count << " failed" << std::endl;

    return Status::OK;
}

Status GrpcGatewayService::HealthCheck(
    ServerContext* /*context*/,
    const gateway::HealthCheckRequest* /*request*/,
    gateway::HealthCheckResponse* response) {

    auto& conn_mgr = gateway::ConnectionManager::instance();

    response->set_healthy(true);
    response->set_active_connections(conn_mgr.active_count());
    response->set_total_connections(conn_mgr.total_count());
    response->set_cpu_usage(0.0);  // TODO: 实现 CPU 使用率监控
    response->set_memory_usage(0.0);  // TODO: 实现内存使用率监控

    return Status::OK;
}

Status GrpcGatewayService::GetNodeStats(
    ServerContext* /*context*/,
    const gateway::NodeStatsRequest* request,
    gateway::NodeStatsResponse* response) {

    auto& conn_mgr = gateway::ConnectionManager::instance();
    auto& metrics = Metrics::instance();

    response->set_node_id(request->node_id());
    response->set_active_connections(conn_mgr.active_count());
    response->set_total_connections(conn_mgr.total_count());
    response->set_messages_received(metrics.total_messages_received());
    response->set_messages_sent(metrics.total_messages_sent());

    // 计算运行时间
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    response->set_uptime_seconds(uptime.count());

    return Status::OK;
}

// ============================================================================
// GrpcGatewayServer Implementation
// ============================================================================

GrpcGatewayServer::GrpcGatewayServer(uint16_t port)
    : port_(port)
    , service_(std::make_unique<GrpcGatewayService>()) {
}

GrpcGatewayServer::~GrpcGatewayServer() {
    stop();
}

void GrpcGatewayServer::start() {
    std::string server_address = "0.0.0.0:" + std::to_string(port_);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());

    server_ = builder.BuildAndStart();
    std::cout << "gRPC server listening on " << server_address << std::endl;
}

void GrpcGatewayServer::stop() {
    if (server_) {
        server_->Shutdown();
        std::cout << "gRPC server stopped" << std::endl;
    }
}

void GrpcGatewayServer::wait() {
    if (server_) {
        server_->Wait();
    }
}
