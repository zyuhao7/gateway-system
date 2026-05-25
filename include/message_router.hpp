/**
 * @file message_router.hpp
 * @brief 消息路由器，负责本地和跨节点的消息投递
 *
 * 根据用户 ID 使用一致性哈希确定目标节点，
 * 如果用户在本地则直接投递，否则转发到远程节点。
 */

#pragma once

#include "node_registry.hpp"
#include "grpc_gateway_client.hpp"
#include <boost/asio.hpp>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <shared_mutex>

namespace asio = boost::asio;

/**
 * @class MessageRouter
 * @brief 消息路由器
 *
 * 功能：
 * - 根据用户 ID 查找目标节点
 * - 本地用户直接投递
 * - 远程用户转发到对应节点
 * - 维护本地用户列表
 *
 * 使用场景：
 * - 用户 A（在 node1）发消息给用户 B（在 node2）
 * - node1 的 MessageRouter 查询 B 在哪个节点
 * - 发现 B 在 node2，通过网络转发消息
 * - node2 收到消息后投递给 B
 */
class MessageRouter {
public:
    /**
     * @struct Message
     * @brief 消息结构
     */
    struct Message {
        std::string from_user_id;   ///< 发送者用户 ID
        std::string to_user_id;     ///< 接收者用户 ID
        std::string content;        ///< 消息内容
        uint64_t timestamp;         ///< 时间戳
    };

    /**
     * @brief 消息处理回调函数类型
     *
     * 当收到远程节点转发的消息时调用
     */
    using MessageHandler = std::function<void(const Message&)>;

    /**
     * @brief 构造函数
     * @param io_context Asio IO 上下文
     * @param registry 节点注册中心
     * @param grpc_client gRPC 客户端
     * @param local_node_id 本地节点ID
     */
    explicit MessageRouter(asio::io_context& io_context,
                          std::shared_ptr<NodeRegistry> registry,
                          std::shared_ptr<GrpcGatewayClient> grpc_client,
                          const std::string& local_node_id);

    /**
     * @brief 路由消息到目标用户
     * @param msg 消息
     * @return 成功返回 true
     *
     * 流程：
     * 1. 使用一致性哈希查找目标用户所在节点
     * 2. 如果在本地，直接投递
     * 3. 如果在远程，通过网络转发
     */
    asio::awaitable<bool> route_message(const Message& msg);

    /**
     * @brief 发送消息给本地用户
     * @param user_id 用户 ID
     * @param content 消息内容
     * @return 成功返回 true
     *
     * 查找用户对应的连接，通过连接发送消息
     */
    bool send_to_local_user(const std::string& user_id, const std::string& content);

    /**
     * @brief 注册本地用户
     * @param user_id 用户 ID
     * @param conn_id 连接 ID
     *
     * 当用户连接到本节点时调用，建立 user_id -> conn_id 映射
     */
    void register_local_user(const std::string& user_id, uint64_t conn_id);

    /**
     * @brief 注销本地用户
     * @param user_id 用户 ID
     *
     * 当用户断开连接时调用
     */
    void unregister_local_user(const std::string& user_id);

    /**
     * @brief 设置消息处理回调
     * @param handler 回调函数
     *
     * 用于处理从远程节点转发过来的消息
     */
    void set_message_handler(MessageHandler handler);

    /**
     * @brief 获取用户所在的节点
     * @param user_id 用户 ID
     * @return 节点 ID
     */
    std::string get_node_for_user(const std::string& user_id) const;

private:
    /**
     * @brief 发送消息到远程节点
     * @param node_id 目标节点 ID
     * @param msg 消息
     * @return 成功返回 true
     *
     * 使用 gRPC 转发消息到远程节点
     */
    asio::awaitable<bool> send_to_remote_node(const std::string& node_id,
                                              const Message& msg);

    asio::io_context& io_context_;                  ///< Asio IO 上下文
    std::shared_ptr<NodeRegistry> registry_;        ///< 节点注册中心
    std::shared_ptr<GrpcGatewayClient> grpc_client_; ///< gRPC 客户端
    std::string local_node_id_;                     ///< 本地节点ID
    MessageHandler message_handler_;                ///< 消息处理回调

    mutable std::shared_mutex mutex_;               ///< 保护本地用户列表
    std::map<std::string, uint64_t> local_users_;   ///< 本地用户映射：user_id -> conn_id
};
