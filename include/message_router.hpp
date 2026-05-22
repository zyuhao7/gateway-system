#pragma once

#include "node_registry.hpp"
#include <boost/asio.hpp>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <shared_mutex>

namespace asio = boost::asio;

class MessageRouter {
public:
    struct Message {
        std::string from_user_id;
        std::string to_user_id;
        std::string content;
        uint64_t timestamp;
    };

    using MessageHandler = std::function<void(const Message&)>;

    explicit MessageRouter(asio::io_context& io_context,
                          std::shared_ptr<NodeRegistry> registry);

    // Route message to target user
    asio::awaitable<bool> route_message(const Message& msg);

    // Send message to local user
    bool send_to_local_user(const std::string& user_id, const std::string& content);

    // Register local user
    void register_local_user(const std::string& user_id, uint64_t conn_id);

    // Unregister local user
    void unregister_local_user(const std::string& user_id);

    // Set message handler for incoming messages
    void set_message_handler(MessageHandler handler);

    // Get node for user
    std::string get_node_for_user(const std::string& user_id) const;

private:
    asio::awaitable<bool> send_to_remote_node(const std::string& node_id,
                                              const Message& msg);

    asio::io_context& io_context_;
    std::shared_ptr<NodeRegistry> registry_;
    MessageHandler message_handler_;

    mutable std::shared_mutex mutex_;
    std::map<std::string, uint64_t> local_users_;  // user_id -> conn_id
};
