#pragma once

#include "consistent_hash.hpp"
#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <map>
#include <shared_mutex>

class NodeRegistry {
public:
    struct NodeInfo {
        std::string node_id;
        std::string host;
        uint16_t port;
        std::chrono::system_clock::time_point last_heartbeat;
        bool is_alive{true};
    };

    using NodeChangeCallback = std::function<void(const std::string& node_id, bool added)>;

    explicit NodeRegistry(const std::string& redis_host = "localhost",
                         uint16_t redis_port = 6379);
    ~NodeRegistry();

    // Register this node
    bool register_node(const std::string& node_id,
                      const std::string& host,
                      uint16_t port);

    // Unregister this node
    bool unregister_node(const std::string& node_id);

    // Send heartbeat
    bool heartbeat(const std::string& node_id);

    // Get node for a given key (using consistent hash)
    std::string get_node_for_key(const std::string& key) const;

    // Get node info
    NodeInfo get_node_info(const std::string& node_id) const;

    // Get all alive nodes
    std::vector<std::string> get_alive_nodes() const;

    // Set callback for node changes
    void set_node_change_callback(NodeChangeCallback callback);

    // Start background tasks (heartbeat, node discovery)
    void start();
    void stop();

private:
    void heartbeat_loop();
    void discovery_loop();
    void check_node_health();

    std::string redis_host_;
    uint16_t redis_port_;
    std::string local_node_id_;

    mutable std::shared_mutex mutex_;
    std::map<std::string, NodeInfo> nodes_;
    std::unique_ptr<ConsistentHash> hash_ring_;

    NodeChangeCallback node_change_callback_;

    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    std::thread discovery_thread_;

    static constexpr int HEARTBEAT_INTERVAL_SEC = 5;
    static constexpr int NODE_TIMEOUT_SEC = 15;
};
