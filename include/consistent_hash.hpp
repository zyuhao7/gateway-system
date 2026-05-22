#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <shared_mutex>

class ConsistentHash {
public:
    explicit ConsistentHash(size_t virtual_nodes = 150);

    void add_node(const std::string& node);
    void remove_node(const std::string& node);
    std::string get_node(const std::string& key) const;

    std::vector<std::string> get_all_nodes() const;
    size_t node_count() const;

private:
    uint32_t hash(const std::string& key) const;

    size_t virtual_nodes_;
    mutable std::shared_mutex mutex_;
    std::map<uint32_t, std::string> ring_;
    std::vector<std::string> nodes_;
};
