/**
 * @file consistent_hash.hpp
 * @brief 一致性哈希实现，用于分布式节点的负载均衡
 *
 * 使用 MurmurHash3 算法和虚拟节点技术，实现均匀的负载分布。
 * 当节点增减时，只影响部分 key 的映射，最小化数据迁移。
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <shared_mutex>

/**
 * @class ConsistentHash
 * @brief 一致性哈希环实现
 *
 * 特性：
 * - 使用虚拟节点提高分布均匀性
 * - 线程安全（读写锁）
 * - 支持动态添加/删除节点
 *
 * 使用场景：
 * - 分布式缓存（Memcached、Redis 集群）
 * - 负载均衡（用户到网关节点的映射）
 * - 分布式存储（数据分片）
 */
class ConsistentHash {
public:
    /**
     * @brief 构造函数
     * @param virtual_nodes 每个物理节点对应的虚拟节点数量（默认 150）
     *
     * 虚拟节点越多，分布越均匀，但内存占用和查找时间也会增加。
     * 推荐值：100-200
     */
    explicit ConsistentHash(size_t virtual_nodes = 150);

    /**
     * @brief 添加节点到哈希环
     * @param node 节点标识符（如 "node1", "192.168.1.100:8080"）
     *
     * 会创建 virtual_nodes 个虚拟节点分布在哈希环上。
     * 如果节点已存在，则忽略。
     */
    void add_node(const std::string& node);

    /**
     * @brief 从哈希环移除节点
     * @param node 节点标识符
     *
     * 移除该节点的所有虚拟节点。
     * 该节点负责的 key 会自动重新分配到其他节点。
     */
    void remove_node(const std::string& node);

    /**
     * @brief 根据 key 查找对应的节点
     * @param key 键值（如用户 ID、缓存 key）
     * @return 负责该 key 的节点标识符，如果没有节点则返回空字符串
     *
     * 算法：
     * 1. 计算 key 的 hash 值
     * 2. 在哈希环上顺时针查找第一个虚拟节点
     * 3. 返回该虚拟节点对应的物理节点
     */
    std::string get_node(const std::string& key) const;

    /**
     * @brief 获取所有节点列表
     * @return 节点标识符列表
     */
    std::vector<std::string> get_all_nodes() const;

    /**
     * @brief 获取节点数量
     * @return 物理节点数量（不包括虚拟节点）
     */
    size_t node_count() const;

private:
    /**
     * @brief 计算字符串的哈希值
     * @param key 输入字符串
     * @return 32 位哈希值
     *
     * 使用 MurmurHash3 算法，特点：
     * - 速度快
     * - 分布均匀
     * - 雪崩效应好（输入微小变化导致输出剧烈变化）
     */
    uint32_t hash(const std::string& key) const;

    size_t virtual_nodes_;                          ///< 每个物理节点的虚拟节点数量
    mutable std::shared_mutex mutex_;               ///< 读写锁，保护哈希环
    std::map<uint32_t, std::string> ring_;          ///< 哈希环：hash 值 -> 物理节点
    std::vector<std::string> nodes_;                ///< 物理节点列表
};
