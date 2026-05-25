/**
 * @file node_registry_redis.cpp
 * @brief NodeRegistry Redis 集成实现
 */

#include "node_registry.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <sstream>
#include <mutex>
#include <shared_mutex>

bool NodeRegistry::connect_redis() {
    struct timeval timeout = { 2, 0 };  // 2 seconds timeout
    redis_context_ = redisConnectWithTimeout(redis_host_.c_str(), redis_port_, timeout);

    if (redis_context_ == nullptr || redis_context_->err) {
        if (redis_context_) {
            std::cerr << "Redis connection error: " << redis_context_->errstr << std::endl;
            redisFree(redis_context_);
            redis_context_ = nullptr;
        } else {
            std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
        }
        return false;
    }

    std::cout << "Connected to Redis at " << redis_host_ << ":" << redis_port_ << std::endl;
    return true;
}

void NodeRegistry::disconnect_redis() {
    if (redis_context_) {
        redisFree(redis_context_);
        redis_context_ = nullptr;
        std::cout << "Disconnected from Redis" << std::endl;
    }
}

bool NodeRegistry::save_node_to_redis(const std::string& node_id, const NodeInfo& info) {
    if (!redis_context_) {
        return false;
    }

    // 保存节点信息到 hash: gateway:nodes
    // 格式: HSET gateway:nodes <node_id> "<host>:<port>"
    std::ostringstream value;
    value << info.host << ":" << info.port;

    redisReply* reply = (redisReply*)redisCommand(
        redis_context_,
        "HSET gateway:nodes %s %s",
        node_id.c_str(),
        value.str().c_str()
    );

    if (!reply) {
        std::cerr << "Redis HSET failed: " << redis_context_->errstr << std::endl;
        return false;
    }

    freeReplyObject(reply);

    // 设置心跳 key 的过期时间
    // 格式: SETEX gateway:heartbeat:<node_id> <timeout> "alive"
    std::string heartbeat_key = "gateway:heartbeat:" + node_id;
    reply = (redisReply*)redisCommand(
        redis_context_,
        "SETEX %s %d alive",
        heartbeat_key.c_str(),
        NODE_TIMEOUT_SEC
    );

    if (!reply) {
        std::cerr << "Redis SETEX failed: " << redis_context_->errstr << std::endl;
        return false;
    }

    freeReplyObject(reply);
    return true;
}

bool NodeRegistry::delete_node_from_redis(const std::string& node_id) {
    if (!redis_context_) {
        return false;
    }

    // 从 hash 中删除节点
    redisReply* reply = (redisReply*)redisCommand(
        redis_context_,
        "HDEL gateway:nodes %s",
        node_id.c_str()
    );

    if (!reply) {
        std::cerr << "Redis HDEL failed: " << redis_context_->errstr << std::endl;
        return false;
    }

    freeReplyObject(reply);

    // 删除心跳 key
    std::string heartbeat_key = "gateway:heartbeat:" + node_id;
    reply = (redisReply*)redisCommand(
        redis_context_,
        "DEL %s",
        heartbeat_key.c_str()
    );

    if (!reply) {
        std::cerr << "Redis DEL failed: " << redis_context_->errstr << std::endl;
        return false;
    }

    freeReplyObject(reply);
    return true;
}

bool NodeRegistry::update_heartbeat_in_redis(const std::string& node_id) {
    if (!redis_context_) {
        return false;
    }

    // 更新心跳 key 的过期时间
    std::string heartbeat_key = "gateway:heartbeat:" + node_id;
    redisReply* reply = (redisReply*)redisCommand(
        redis_context_,
        "EXPIRE %s %d",
        heartbeat_key.c_str(),
        NODE_TIMEOUT_SEC
    );

    if (!reply) {
        std::cerr << "Redis EXPIRE failed: " << redis_context_->errstr << std::endl;
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    return success;
}

bool NodeRegistry::load_nodes_from_redis() {
    if (!redis_context_) {
        return false;
    }

    // 获取所有节点: HGETALL gateway:nodes
    redisReply* reply = (redisReply*)redisCommand(
        redis_context_,
        "HGETALL gateway:nodes"
    );

    if (!reply) {
        std::cerr << "Redis HGETALL failed: " << redis_context_->errstr << std::endl;
        return false;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return false;
    }

    std::cout << "Loading nodes from Redis, found " << (reply->elements / 2) << " nodes" << std::endl;

    // HGETALL 返回 [key1, value1, key2, value2, ...]
    for (size_t i = 0; i < reply->elements; i += 2) {
        std::string node_id = reply->element[i]->str;
        std::string value = reply->element[i + 1]->str;

        std::cout << "  Found in Redis: " << node_id << " = " << value << std::endl;

        // 跳过本地节点
        if (node_id == local_node_id_) {
            std::cout << "    Skipping local node" << std::endl;
            continue;
        }

        // 解析 host:port
        size_t colon_pos = value.find(':');
        if (colon_pos == std::string::npos) {
            std::cout << "    Invalid format, skipping" << std::endl;
            continue;
        }

        std::string host = value.substr(0, colon_pos);
        uint16_t port = static_cast<uint16_t>(std::stoi(value.substr(colon_pos + 1)));

        // 检查节点是否已存在
        std::unique_lock lock(mutex_);
        if (nodes_.find(node_id) == nodes_.end()) {
            // 新节点，添加到本地
            NodeInfo info;
            info.node_id = node_id;
            info.host = host;
            info.port = port;
            info.last_heartbeat = std::chrono::system_clock::now();
            info.is_alive = true;

            nodes_[node_id] = info;
            hash_ring_->add_node(node_id);

            std::cout << "Discovered new node from Redis: " << node_id
                      << " (" << host << ":" << port << ")" << std::endl;

            // 触发回调
            lock.unlock();
            if (node_change_callback_) {
                node_change_callback_(node_id, true);
            }
        } else {
            // 已存在的节点，更新心跳时间
            std::cout << "    Node already exists, updating heartbeat" << std::endl;
            nodes_[node_id].last_heartbeat = std::chrono::system_clock::now();
            nodes_[node_id].is_alive = true;
        }
    }

    freeReplyObject(reply);
    return true;
}
