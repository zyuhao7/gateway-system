/**
 * @file node_registry.cpp
 * @brief 节点注册中心实现
 */

#include "node_registry.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <sstream>

NodeRegistry::NodeRegistry(const std::string& redis_host, uint16_t redis_port)
    : redis_host_(redis_host)
    , redis_port_(redis_port)
    , hash_ring_(std::make_unique<ConsistentHash>(150)) {
    connect_redis();
}

NodeRegistry::~NodeRegistry() {
    stop();
    disconnect_redis();
}

bool NodeRegistry::register_node(const std::string& node_id,
                                 const std::string& host,
                                 uint16_t port) {
    std::unique_lock lock(mutex_);

    local_node_id_ = node_id;

    // 创建节点信息
    NodeInfo info;
    info.node_id = node_id;
    info.host = host;
    info.port = port;
    info.last_heartbeat = std::chrono::system_clock::now();
    info.is_alive = true;

    nodes_[node_id] = info;
    hash_ring_->add_node(node_id);  // 添加到一致性哈希环

    std::cout << "Node registered: " << node_id
              << " (" << host << ":" << port << ")" << std::endl;

    // 将节点信息持久化到 Redis
    lock.unlock();  // 释放锁后再调用 Redis
    save_node_to_redis(node_id, info);

    return true;
}

bool NodeRegistry::unregister_node(const std::string& node_id) {
    std::unique_lock lock(mutex_);

    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }

    hash_ring_->remove_node(node_id);  // 从哈希环移除
    nodes_.erase(it);

    std::cout << "Node unregistered: " << node_id << std::endl;

    // 从 Redis 删除节点信息
    lock.unlock();  // 释放锁后再调用 Redis
    delete_node_from_redis(node_id);

    // 通知上层节点已离开
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

    // 更新最后心跳时间
    it->second.last_heartbeat = std::chrono::system_clock::now();
    it->second.is_alive = true;

    // 更新 Redis 中的心跳时间戳
    lock.unlock();  // 释放锁后再调用 Redis
    update_heartbeat_in_redis(node_id);

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
        return;  // 已经在运行
    }

    // 启动心跳线程
    heartbeat_thread_ = std::thread([this]() { heartbeat_loop(); });

    // 启动节点发现线程
    discovery_thread_ = std::thread([this]() { discovery_loop(); });

    std::cout << "NodeRegistry started" << std::endl;
}

void NodeRegistry::stop() {
    if (!running_.exchange(false)) {
        return;  // 已经停止
    }

    // 等待线程结束
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }

    // 注销本节点
    if (!local_node_id_.empty()) {
        unregister_node(local_node_id_);
    }

    std::cout << "NodeRegistry stopped" << std::endl;
}

/**
 * 心跳线程主循环
 *
 * 每 5 秒为本节点发送一次心跳，表明节点仍然存活。
 * 心跳信息会更新到本地内存和 Redis。
 */
void NodeRegistry::heartbeat_loop() {
    while (running_) {
        if (!local_node_id_.empty()) {
            heartbeat(local_node_id_);
        }

        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL_SEC));
    }
}

/**
 * 节点发现线程主循环
 *
 * 每 5 秒执行：
 * 1. 从 Redis 发现新加入的节点
 * 2. 检查所有节点的健康状态
 */
void NodeRegistry::discovery_loop() {
    while (running_) {
        // 从 Redis 发现新节点
        load_nodes_from_redis();

        check_node_health();

        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL_SEC));
    }
}

/**
 * 检查所有节点的健康状态
 *
 * 遍历所有节点，检查距离上次心跳的时间：
 * - 如果超过 15 秒，标记为失联
 * - 从一致性哈希环中移除
 * - 触发节点变化回调，通知上层应用
 *
 * 这样可以自动摘除故障节点，避免将请求路由到已经挂掉的节点。
 */
void NodeRegistry::check_node_health() {
    std::unique_lock lock(mutex_);

    auto now = std::chrono::system_clock::now();

    for (auto& [node_id, info] : nodes_) {
        // 跳过本地节点（本地节点由自己的心跳线程维护）
        if (node_id == local_node_id_) {
            continue;
        }

        // 计算距离上次心跳的时间
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - info.last_heartbeat).count();

        // 超过 15 秒没有心跳 = 节点失联
        if (elapsed > NODE_TIMEOUT_SEC && info.is_alive) {
            info.is_alive = false;
            hash_ring_->remove_node(node_id);  // 从哈希环移除

            std::cout << "Node timeout: " << node_id << std::endl;

            // 通知上层节点已失联
            if (node_change_callback_) {
                node_change_callback_(node_id, false);
            }
        }
    }
}
