/**
 * @file node_registry.hpp
 * @brief 节点注册与发现服务
 *
 * 管理分布式网关集群中的所有节点，提供：
 * - 节点注册/注销
 * - 心跳检测
 * - 故障节点自动摘除
 * - 基于一致性哈希的节点选择
 */

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

// Forward declaration
struct redisContext;

/**
 * @class NodeRegistry
 * @brief 节点注册中心
 *
 * 核心功能：
 * - 维护集群中所有网关节点的信息
 * - 定期发送心跳，检测节点健康状态
 * - 使用一致性哈希分配用户到节点
 * - 支持 Redis 持久化（待实现）
 *
 * 工作流程：
 * 1. 节点启动时调用 register_node() 注册自己
 * 2. 后台线程每 5 秒发送心跳
 * 3. 后台线程每 5 秒检查其他节点健康状态
 * 4. 超过 15 秒无心跳的节点被标记为失联并从哈希环移除
 */
class NodeRegistry {
public:
    /**
     * @struct NodeInfo
     * @brief 节点信息
     */
    struct NodeInfo {
        std::string node_id;                                    ///< 节点唯一标识
        std::string host;                                       ///< 主机地址
        uint16_t port;                                          ///< 端口号
        std::chrono::system_clock::time_point last_heartbeat;  ///< 最后心跳时间
        bool is_alive{true};                                    ///< 是否存活
    };

    /**
     * @brief 节点变化回调函数类型
     * @param node_id 节点 ID
     * @param added true=节点加入，false=节点离开
     */
    using NodeChangeCallback = std::function<void(const std::string& node_id, bool added)>;

    /**
     * @brief 构造函数
     * @param redis_host Redis 服务器地址（用于持久化节点信息）
     * @param redis_port Redis 端口
     */
    explicit NodeRegistry(const std::string& redis_host = "localhost",
                         uint16_t redis_port = 6379);
    ~NodeRegistry();

    /**
     * @brief 注册本节点
     * @param node_id 节点唯一标识（如 "gateway-1"）
     * @param host 本节点的主机地址
     * @param port 本节点的端口
     * @return 成功返回 true
     *
     * 注册后会：
     * 1. 将节点信息存储到本地和 Redis
     * 2. 添加到一致性哈希环
     * 3. 开始发送心跳
     */
    bool register_node(const std::string& node_id,
                      const std::string& host,
                      uint16_t port);

    /**
     * @brief 注销节点
     * @param node_id 节点 ID
     * @return 成功返回 true
     *
     * 会从哈希环和 Redis 中移除该节点
     */
    bool unregister_node(const std::string& node_id);

    /**
     * @brief 发送心跳
     * @param node_id 节点 ID
     * @return 成功返回 true
     *
     * 更新节点的最后心跳时间，表明节点仍然存活
     */
    bool heartbeat(const std::string& node_id);

    /**
     * @brief 根据 key 获取对应的节点
     * @param key 键值（通常是用户 ID）
     * @return 负责该 key 的节点 ID
     *
     * 使用一致性哈希算法确定节点
     */
    std::string get_node_for_key(const std::string& key) const;

    /**
     * @brief 获取节点信息
     * @param node_id 节点 ID
     * @return 节点信息，如果不存在则返回空结构
     */
    NodeInfo get_node_info(const std::string& node_id) const;

    /**
     * @brief 获取所有存活的节点
     * @return 存活节点 ID 列表
     */
    std::vector<std::string> get_alive_nodes() const;

    /**
     * @brief 设置节点变化回调
     * @param callback 回调函数
     *
     * 当节点加入或离开时会触发回调
     */
    void set_node_change_callback(NodeChangeCallback callback);

    /**
     * @brief 启动后台任务
     *
     * 启动两个后台线程：
     * - 心跳线程：每 5 秒发送心跳
     * - 发现线程：每 5 秒检查节点健康状态
     */
    void start();

    /**
     * @brief 停止后台任务
     *
     * 停止所有后台线程并注销本节点
     */
    void stop();

private:
    /**
     * @brief 连接到 Redis
     * @return 成功返回 true
     */
    bool connect_redis();

    /**
     * @brief 断开 Redis 连接
     */
    void disconnect_redis();

    /**
     * @brief 从 Redis 加载所有节点
     * @return 成功返回 true
     */
    bool load_nodes_from_redis();

    /**
     * @brief 将节点信息保存到 Redis
     * @param node_id 节点 ID
     * @param info 节点信息
     * @return 成功返回 true
     */
    bool save_node_to_redis(const std::string& node_id, const NodeInfo& info);

    /**
     * @brief 从 Redis 删除节点
     * @param node_id 节点 ID
     * @return 成功返回 true
     */
    bool delete_node_from_redis(const std::string& node_id);

    /**
     * @brief 更新节点心跳到 Redis
     * @param node_id 节点 ID
     * @return 成功返回 true
     */
    bool update_heartbeat_in_redis(const std::string& node_id);

    /**
     * @brief 心跳循环（后台线程）
     *
     * 每 5 秒为本节点发送一次心跳
     */
    void heartbeat_loop();

    /**
     * @brief 节点发现循环（后台线程）
     *
     * 每 5 秒执行：
     * 1. 从 Redis 发现新节点
     * 2. 检查所有节点的健康状态
     */
    void discovery_loop();

    /**
     * @brief 检查节点健康状态
     *
     * 遍历所有节点，如果超过 15 秒没有心跳：
     * 1. 标记为失联
     * 2. 从哈希环移除
     * 3. 触发节点变化回调
     */
    void check_node_health();

    std::string redis_host_;                        ///< Redis 主机地址
    uint16_t redis_port_;                           ///< Redis 端口
    redisContext* redis_context_{nullptr};          ///< Redis 连接上下文
    std::string local_node_id_;                     ///< 本地节点 ID

    mutable std::shared_mutex mutex_;               ///< 保护节点信息的读写锁
    std::map<std::string, NodeInfo> nodes_;         ///< 所有节点信息
    std::unique_ptr<ConsistentHash> hash_ring_;     ///< 一致性哈希环

    NodeChangeCallback node_change_callback_;       ///< 节点变化回调

    std::atomic<bool> running_{false};              ///< 后台线程运行标志
    std::thread heartbeat_thread_;                  ///< 心跳线程
    std::thread discovery_thread_;                  ///< 发现线程

    static constexpr int HEARTBEAT_INTERVAL_SEC = 5;  ///< 心跳间隔（秒）
    static constexpr int NODE_TIMEOUT_SEC = 15;       ///< 节点超时时间（秒）
};
