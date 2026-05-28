/**
 * @file protocol.hpp
 * @brief 网关消息协议定义
 */

#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <boost/json.hpp>

namespace gateway {

namespace json = boost::json;

// 消息类型
enum class MessageType {
    // 连接管理
    CONNECT,        // 连接请求
    DISCONNECT,     // 断开连接
    HEARTBEAT,      // 心跳

    // 消息传输
    MESSAGE,        // 普通消息
    BROADCAST,      // 广播消息

    // 确认
    ACK,            // 确认
    NACK,           // 否定确认

    // 查询
    QUERY_STATS,    // 查询统计
    QUERY_HISTORY,  // 查询历史

    // 响应
    RESPONSE,       // 通用响应
    ERROR,          // 错误响应

    UNKNOWN         // 未知类型
};

// 消息优先级
enum class MessagePriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    URGENT = 3
};

// 消息协议
struct Message {
    // 消息头
    std::string message_id;         // 消息ID（唯一）
    MessageType type;               // 消息类型
    int64_t timestamp;              // 时间戳（微秒）
    MessagePriority priority;       // 优先级

    // 发送者和接收者
    std::string from_user;          // 发送者
    std::string to_user;            // 接收者（空表示广播）

    // 消息内容
    std::string content;            // 消息内容
    json::object metadata;          // 元数据

    // 确认相关
    std::string ack_id;             // 需要确认的消息ID
    bool require_ack;               // 是否需要确认

    // 序列化和反序列化
    std::string serialize() const;
    static std::optional<Message> deserialize(const std::string& data);

    // 辅助方法
    static std::string generate_message_id();
    static int64_t current_timestamp();
    static std::string type_to_string(MessageType type);
    static MessageType string_to_type(const std::string& str);
};

// 连接消息
struct ConnectMessage {
    std::string user_id;
    std::string token;
    json::object client_info;

    Message to_message() const;
    static std::optional<ConnectMessage> from_message(const Message& msg);
};

// 心跳消息
struct HeartbeatMessage {
    int64_t client_timestamp;

    Message to_message() const;
    static std::optional<HeartbeatMessage> from_message(const Message& msg);
};

// 普通消息
struct ChatMessage {
    std::string to_user;
    std::string content;
    MessagePriority priority;
    bool require_ack;

    Message to_message(const std::string& from_user) const;
    static std::optional<ChatMessage> from_message(const Message& msg);
};

// ACK 消息
struct AckMessage {
    std::string ack_message_id;
    bool success;
    std::string error_message;

    Message to_message() const;
    static std::optional<AckMessage> from_message(const Message& msg);
};

// 响应消息
struct ResponseMessage {
    bool success;
    std::string message;
    json::object data;

    Message to_message() const;
    static std::optional<ResponseMessage> from_message(const Message& msg);
};

}
