#include "consistent_hash.hpp"
#include <algorithm>
#include <mutex>

ConsistentHash::ConsistentHash(size_t virtual_nodes)
    : virtual_nodes_(virtual_nodes) {
}

uint32_t ConsistentHash::hash(const std::string& key) const {
    // MurmurHash3 32-bit
    const uint32_t seed = 0x9747b28c;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    uint32_t h = seed ^ key.length();
    const unsigned char* data = reinterpret_cast<const unsigned char*>(key.data());
    size_t len = key.length();

    while (len >= 4) {
        uint32_t k = *reinterpret_cast<const uint32_t*>(data);
        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len) {
        case 3: h ^= data[2] << 16; [[fallthrough]];
        case 2: h ^= data[1] << 8;  [[fallthrough]];
        case 1: h ^= data[0];
                h *= m;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

void ConsistentHash::add_node(const std::string& node) {
    std::unique_lock lock(mutex_);

    // Check if node already exists
    if (std::find(nodes_.begin(), nodes_.end(), node) != nodes_.end()) {
        return;
    }

    nodes_.push_back(node);

    // Add virtual nodes
    for (size_t i = 0; i < virtual_nodes_; ++i) {
        std::string vnode_key = node + "#" + std::to_string(i);
        uint32_t hash_value = hash(vnode_key);
        ring_[hash_value] = node;
    }
}

void ConsistentHash::remove_node(const std::string& node) {
    std::unique_lock lock(mutex_);

    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it == nodes_.end()) {
        return;
    }

    nodes_.erase(it);

    // Remove virtual nodes
    for (size_t i = 0; i < virtual_nodes_; ++i) {
        std::string vnode_key = node + "#" + std::to_string(i);
        uint32_t hash_value = hash(vnode_key);
        ring_.erase(hash_value);
    }
}

std::string ConsistentHash::get_node(const std::string& key) const {
    std::shared_lock lock(mutex_);

    if (ring_.empty()) {
        return "";
    }

    uint32_t hash_value = hash(key);

    // Find the first node >= hash_value
    auto it = ring_.lower_bound(hash_value);

    // Wrap around if necessary
    if (it == ring_.end()) {
        it = ring_.begin();
    }

    return it->second;
}

std::vector<std::string> ConsistentHash::get_all_nodes() const {
    std::shared_lock lock(mutex_);
    return nodes_;
}

size_t ConsistentHash::node_count() const {
    std::shared_lock lock(mutex_);
    return nodes_.size();
}
