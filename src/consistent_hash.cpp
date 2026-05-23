/**
 * @file consistent_hash.cpp
 * @brief 一致性哈希实现
 */

#include "consistent_hash.hpp"
#include <algorithm>
#include <mutex>

ConsistentHash::ConsistentHash(size_t virtual_nodes)
    : virtual_nodes_(virtual_nodes) {
}

/**
 * MurmurHash3 32-bit 实现
 *
 * 算法特点：
 * - 非加密哈希，速度快
 * - 良好的雪崩效应
 * - 均匀分布
 *
 * 参考：https://github.com/aappleby/smhasher
 */
uint32_t ConsistentHash::hash(const std::string& key) const {
    // MurmurHash3 常量
    const uint32_t seed = 0x9747b28c;  // 随机种子
    const uint32_t m = 0x5bd1e995;     // 混合常数
    const int r = 24;                   // 位移量

    uint32_t h = seed ^ key.length();
    const unsigned char* data = reinterpret_cast<const unsigned char*>(key.data());
    size_t len = key.length();

    // 每次处理 4 字节
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

    // 处理剩余字节（0-3 字节）
    switch (len) {
        case 3: h ^= data[2] << 16; [[fallthrough]];
        case 2: h ^= data[1] << 8;  [[fallthrough]];
        case 1: h ^= data[0];
                h *= m;
    }

    // 最终混合
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

void ConsistentHash::add_node(const std::string& node) {
    std::unique_lock lock(mutex_);

    // 检查节点是否已存在
    if (std::find(nodes_.begin(), nodes_.end(), node) != nodes_.end()) {
        return;
    }

    nodes_.push_back(node);

    // 为该物理节点创建多个虚拟节点
    // 虚拟节点命名：node#0, node#1, ..., node#N
    for (size_t i = 0; i < virtual_nodes_; ++i) {
        std::string vnode_key = node + "#" + std::to_string(i);
        uint32_t hash_value = hash(vnode_key);
        ring_[hash_value] = node;  // 虚拟节点指向物理节点
    }
}

void ConsistentHash::remove_node(const std::string& node) {
    std::unique_lock lock(mutex_);

    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it == nodes_.end()) {
        return;
    }

    nodes_.erase(it);

    // 移除该节点的所有虚拟节点
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

    // 在哈希环上顺时针查找第一个节点
    // lower_bound 返回第一个 >= hash_value 的元素
    auto it = ring_.lower_bound(hash_value);

    // 如果到达环的末尾，则回绕到开头（环形结构）
    if (it == ring_.end()) {
        it = ring_.begin();
    }

    return it->second;  // 返回虚拟节点对应的物理节点
}

std::vector<std::string> ConsistentHash::get_all_nodes() const {
    std::shared_lock lock(mutex_);
    return nodes_;
}

size_t ConsistentHash::node_count() const {
    std::shared_lock lock(mutex_);
    return nodes_.size();
}
