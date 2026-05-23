#include "connection_manager.hpp"
#include "connection.hpp"
#include "metrics.hpp"
#include <iostream>

namespace gateway {

ConnectionManager& ConnectionManager::instance() {
    static ConnectionManager mgr;
    return mgr;
}

void ConnectionManager::add(uint64_t id, std::shared_ptr<Connection> conn) {
    std::unique_lock lock(mutex_);
    connections_[id] = conn;
    active_count_.fetch_add(1, std::memory_order_relaxed);

    // 记录指标
    Metrics::instance().connection_opened();

    std::cout << "ConnectionManager: added connection " << id
              << ", total=" << connections_.size() << std::endl;
}

void ConnectionManager::remove(uint64_t id) {
    std::unique_lock lock(mutex_);
    if (connections_.erase(id)) {
        active_count_.fetch_sub(1, std::memory_order_relaxed);

        // 记录指标
        Metrics::instance().connection_closed();

        std::cout << "ConnectionManager: removed connection " << id
                  << ", total=" << connections_.size() << std::endl;
    }
}

std::shared_ptr<Connection> ConnectionManager::find(uint64_t id) {
    std::shared_lock lock(mutex_);
    auto it = connections_.find(id);
    return (it != connections_.end()) ? it->second : nullptr;
}

size_t ConnectionManager::total_count() const {
    std::shared_lock lock(mutex_);
    return connections_.size();
}

size_t ConnectionManager::active_count() const {
    return active_count_.load(std::memory_order_relaxed);
}

void ConnectionManager::broadcast(const std::string& message) {
    std::shared_lock lock(mutex_);
    std::cout << "Broadcasting to " << connections_.size() << " connections" << std::endl;

    for (const auto& [id, conn] : connections_) {
        if (conn) {
            // 异步发送，不阻塞
            asio::co_spawn(conn->socket().get_executor(),
                          conn->write_message(message),
                          asio::detached);
        }
    }
}

}
