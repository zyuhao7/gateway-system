#pragma once

#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

namespace gateway {

class Connection;

class ConnectionManager {
public:
    static ConnectionManager& instance();

    void add(uint64_t id, std::shared_ptr<Connection> conn);
    void remove(uint64_t id);
    std::shared_ptr<Connection> find(uint64_t id);

    size_t total_count() const;
    size_t active_count() const;

    void broadcast(const std::string& message);

private:
    ConnectionManager() = default;

    mutable std::shared_mutex mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<Connection>> connections_;
    std::atomic<size_t> active_count_{0};
};

}
