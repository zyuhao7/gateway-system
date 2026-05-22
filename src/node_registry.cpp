#include "node_registry.hpp"
#include <iostream>
#include <thread>
#include <mutex>

NodeRegistry::NodeRegistry(const std::string& redis_host, uint16_t redis_port)
    : redis_host_(redis_host)
    , redis_port_(redis_port)
    , hash_ring_(std::make_unique<ConsistentHash>(150)) {
}

NodeRegistry::~NodeRegistry() {
    stop();
}

bool NodeRegistry::register_node(const std::string& node_id,
                                 const std::string& host,
                                 uint16_t port) {
    std::unique_lock lock(mutex_);

    local_node_id_ = node_id;

    NodeInfo info;
    info.node_id = node_id;
    info.host = host;
    info.port = port;
    info.last_heartbeat = std::chrono::system_clock::now();
    info.is_alive = true;

    nodes_[node_id] = info;
    hash_ring_->add_node(node_id);

    std::cout << "Node registered: " << node_id
              << " (" << host << ":" << port << ")" << std::endl;

    // TODO: Register to Redis
    // redis_client.hset("gateway:nodes", node_id, host:port)
    // redis_client.expire("gateway:heartbeat:" + node_id, NODE_TIMEOUT_SEC)

    return true;
}

bool NodeRegistry::unregister_node(const std::string& node_id) {
    std::unique_lock lock(mutex_);

    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }

    hash_ring_->remove_node(node_id);
    nodes_.erase(it);

    std::cout << "Node unregistered: " << node_id << std::endl;

    // TODO: Unregister from Redis
    // redis_client.hdel("gateway:nodes", node_id)
    // redis_client.del("gateway:heartbeat:" + node_id)

    if (node_change_callback_) {
        node_change_callback_(node_id, false);
    }

    return true;
}

bool NodeRegistry::heartbeat(const std::string& node_id) {
    std::unique_lock lock(mutex_);

    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.last_heartbeat = std::chrono::system_clock::now();
    it->second.is_alive = true;

    // TODO: Update Redis heartbeat
    // redis_client.expire("gateway:heartbeat:" + node_id, NODE_TIMEOUT_SEC)

    return true;
}

std::string NodeRegistry::get_node_for_key(const std::string& key) const {
    return hash_ring_->get_node(key);
}

NodeRegistry::NodeInfo NodeRegistry::get_node_info(const std::string& node_id) const {
    std::shared_lock lock(mutex_);

    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        return it->second;
    }

    return NodeInfo{};
}

std::vector<std::string> NodeRegistry::get_alive_nodes() const {
    std::shared_lock lock(mutex_);

    std::vector<std::string> alive_nodes;
    for (const auto& [node_id, info] : nodes_) {
        if (info.is_alive) {
            alive_nodes.push_back(node_id);
        }
    }

    return alive_nodes;
}

void NodeRegistry::set_node_change_callback(NodeChangeCallback callback) {
    node_change_callback_ = std::move(callback);
}

void NodeRegistry::start() {
    if (running_.exchange(true)) {
        return;
    }

    heartbeat_thread_ = std::thread([this]() { heartbeat_loop(); });
    discovery_thread_ = std::thread([this]() { discovery_loop(); });

    std::cout << "NodeRegistry started" << std::endl;
}

void NodeRegistry::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }

    if (!local_node_id_.empty()) {
        unregister_node(local_node_id_);
    }

    std::cout << "NodeRegistry stopped" << std::endl;
}

void NodeRegistry::heartbeat_loop() {
    while (running_) {
        if (!local_node_id_.empty()) {
            heartbeat(local_node_id_);
        }

        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL_SEC));
    }
}

void NodeRegistry::discovery_loop() {
    while (running_) {
        // TODO: Discover nodes from Redis
        // auto nodes = redis_client.hgetall("gateway:nodes")
        // for each node, check if it exists locally
        // if not, add it to hash_ring and trigger callback

        check_node_health();

        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL_SEC));
    }
}

void NodeRegistry::check_node_health() {
    std::unique_lock lock(mutex_);

    auto now = std::chrono::system_clock::now();

    for (auto& [node_id, info] : nodes_) {
        if (node_id == local_node_id_) {
            continue;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - info.last_heartbeat).count();

        if (elapsed > NODE_TIMEOUT_SEC && info.is_alive) {
            info.is_alive = false;
            hash_ring_->remove_node(node_id);

            std::cout << "Node timeout: " << node_id << std::endl;

            if (node_change_callback_) {
                node_change_callback_(node_id, false);
            }
        }
    }
}
