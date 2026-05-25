/**
 * @file grpc_gateway_client.hpp
 * @brief gRPC 网关客户端，用于跨节点通信
 */

#pragma once

#include "proto/gateway.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>

/**
 * @class GrpcGatewayClient
 * @brief gRPC 客户端，管理到其他节点的连接
 */
class GrpcGatewayClient {
public:
    GrpcGatewayClient() = default;
    ~GrpcGatewayClient() = default;

    /**
     * @brief 添加远程节点
     * @param node_id 节点ID
     * @param address 节点地址 (host:port)
     */
    void add_node(const std::string& node_id, const std::string& address);

    /**
     * @brief 移除远程节点
     * @param node_id 节点ID
     */
    void remove_node(const std::string& node_id);

    /**
     * @brief 转发消息到远程节点
     * @param node_id 目标节点ID
     * @param user_id 目标用户ID
     * @param message 消息内容
     * @param from_node 来源节点ID
     * @return 是否成功
     */
    bool forward_message(const std::string& node_id,
                        const std::string& user_id,
                        const std::string& message,
                        const std::string& from_node);

    /**
     * @brief 批量转发消息
     * @param node_id 目标节点ID
     * @param messages 消息列表
     * @return 成功数量
     */
    int forward_message_batch(const std::string& node_id,
                             const std::vector<std::tuple<std::string, std::string>>& messages,
                             const std::string& from_node);

    /**
     * @brief 健康检查
     * @param node_id 节点ID
     * @return 是否健康
     */
    bool health_check(const std::string& node_id);

    /**
     * @brief 获取节点统计信息
     * @param node_id 节点ID
     * @return 统计信息（JSON格式字符串）
     */
    std::string get_node_stats(const std::string& node_id);

private:
    /**
     * @brief 获取或创建到指定节点的stub
     */
    std::shared_ptr<gateway::GatewayService::Stub> get_stub(const std::string& node_id);

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> node_addresses_;  // node_id -> address
    std::unordered_map<std::string, std::shared_ptr<gateway::GatewayService::Stub>> stubs_;
};
