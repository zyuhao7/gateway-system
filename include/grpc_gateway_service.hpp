/**
 * @file grpc_gateway_service.hpp
 * @brief gRPC 网关服务实现
 */

#pragma once

#include "proto/gateway.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

// Forward declarations
class ConnectionManager;
class Metrics;

/**
 * @class GrpcGatewayService
 * @brief gRPC 服务实现，处理跨节点消息转发
 */
class GrpcGatewayService final : public gateway::GatewayService::Service {
public:
    GrpcGatewayService();
    ~GrpcGatewayService() = default;

    /**
     * @brief 转发消息到本节点的用户
     */
    grpc::Status ForwardMessage(
        grpc::ServerContext* context,
        const gateway::ForwardMessageRequest* request,
        gateway::ForwardMessageResponse* response) override;

    /**
     * @brief 批量转发消息
     */
    grpc::Status ForwardMessageBatch(
        grpc::ServerContext* context,
        const gateway::ForwardMessageBatchRequest* request,
        gateway::ForwardMessageBatchResponse* response) override;

    /**
     * @brief 健康检查
     */
    grpc::Status HealthCheck(
        grpc::ServerContext* context,
        const gateway::HealthCheckRequest* request,
        gateway::HealthCheckResponse* response) override;

    /**
     * @brief 获取节点统计信息
     */
    grpc::Status GetNodeStats(
        grpc::ServerContext* context,
        const gateway::NodeStatsRequest* request,
        gateway::NodeStatsResponse* response) override;

private:
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @class GrpcGatewayServer
 * @brief gRPC 服务器封装
 */
class GrpcGatewayServer {
public:
    /**
     * @brief 构造函数
     * @param port gRPC 服务端口
     */
    explicit GrpcGatewayServer(uint16_t port);
    ~GrpcGatewayServer();

    /**
     * @brief 启动 gRPC 服务器
     */
    void start();

    /**
     * @brief 停止 gRPC 服务器
     */
    void stop();

    /**
     * @brief 等待服务器关闭
     */
    void wait();

private:
    uint16_t port_;
    std::unique_ptr<GrpcGatewayService> service_;
    std::unique_ptr<grpc::Server> server_;
};
