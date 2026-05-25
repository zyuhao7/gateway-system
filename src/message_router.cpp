#include "message_router.hpp"
#include "connection_manager.hpp"
#include "connection.hpp"
#include <iostream>

MessageRouter::MessageRouter(asio::io_context& io_context,
                            std::shared_ptr<NodeRegistry> registry,
                            std::shared_ptr<GrpcGatewayClient> grpc_client,
                            const std::string& local_node_id)
    : io_context_(io_context)
    , registry_(std::move(registry))
    , grpc_client_(std::move(grpc_client))
    , local_node_id_(local_node_id) {
}

asio::awaitable<bool> MessageRouter::route_message(const Message& msg) {
    // Determine target node using consistent hash
    std::string target_node = registry_->get_node_for_key(msg.to_user_id);

    if (target_node.empty()) {
        std::cerr << "No node available for user: " << msg.to_user_id << std::endl;
        co_return false;
    }

    // Check if user is on local node
    {
        std::shared_lock lock(mutex_);
        if (local_users_.find(msg.to_user_id) != local_users_.end()) {
            // Local delivery
            bool success = send_to_local_user(msg.to_user_id, msg.content);
            co_return success;
        }
    }

    // Remote delivery
    std::cout << "Routing message to remote node: " << target_node << std::endl;
    bool success = co_await send_to_remote_node(target_node, msg);
    co_return success;
}

bool MessageRouter::send_to_local_user(const std::string& user_id,
                                       const std::string& content) {
    std::shared_lock lock(mutex_);

    auto it = local_users_.find(user_id);
    if (it == local_users_.end()) {
        return false;
    }

    uint64_t conn_id = it->second;
    auto conn = gateway::ConnectionManager::instance().find(conn_id);

    if (!conn) {
        return false;
    }

    // Send message asynchronously
    asio::co_spawn(io_context_,
                  conn->write_message("MSG: " + content + "\n"),
                  asio::detached);

    return true;
}

void MessageRouter::register_local_user(const std::string& user_id, uint64_t conn_id) {
    std::unique_lock lock(mutex_);
    local_users_[user_id] = conn_id;

    std::cout << "User registered locally: " << user_id
              << " (conn_id=" << conn_id << ")" << std::endl;
}

void MessageRouter::unregister_local_user(const std::string& user_id) {
    std::unique_lock lock(mutex_);
    local_users_.erase(user_id);

    std::cout << "User unregistered locally: " << user_id << std::endl;
}

void MessageRouter::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

std::string MessageRouter::get_node_for_user(const std::string& user_id) const {
    return registry_->get_node_for_key(user_id);
}

asio::awaitable<bool> MessageRouter::send_to_remote_node(const std::string& node_id,
                                                         const Message& msg) {
    if (!grpc_client_) {
        std::cerr << "gRPC client not initialized" << std::endl;
        co_return false;
    }

    std::cout << "Forwarding message to node " << node_id
              << " for user " << msg.to_user_id << std::endl;

    // 使用 gRPC 转发消息
    // 注意：gRPC 调用是阻塞的，在生产环境中应该使用异步 gRPC
    bool success = grpc_client_->forward_message(
        node_id,
        msg.to_user_id,
        msg.content,
        local_node_id_
    );

    co_return success;
}
