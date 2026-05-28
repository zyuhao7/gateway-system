/**
 * @file protocol.cpp
 * @brief 网关消息协议实现
 */

#include "protocol.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace gateway {

// 生成唯一消息ID
std::string Message::generate_message_id() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    uint64_t random = dis(gen);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << now
        << std::setw(16) << random;

    return oss.str();
}

// 获取当前时间戳（微秒）
int64_t Message::current_timestamp() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// 消息类型转字符串
std::string Message::type_to_string(MessageType type) {
    switch (type) {
        case MessageType::CONNECT: return "connect";
        case MessageType::DISCONNECT: return "disconnect";
        case MessageType::HEARTBEAT: return "heartbeat";
        case MessageType::MESSAGE: return "message";
        case MessageType::BROADCAST: return "broadcast";
        case MessageType::ACK: return "ack";
        case MessageType::NACK: return "nack";
        case MessageType::QUERY_STATS: return "query_stats";
        case MessageType::QUERY_HISTORY: return "query_history";
        case MessageType::RESPONSE: return "response";
        case MessageType::ERROR: return "error";
        default: return "unknown";
    }
}

// 字符串转消息类型
MessageType Message::string_to_type(const std::string& str) {
    if (str == "connect") return MessageType::CONNECT;
    if (str == "disconnect") return MessageType::DISCONNECT;
    if (str == "heartbeat") return MessageType::HEARTBEAT;
    if (str == "message") return MessageType::MESSAGE;
    if (str == "broadcast") return MessageType::BROADCAST;
    if (str == "ack") return MessageType::ACK;
    if (str == "nack") return MessageType::NACK;
    if (str == "query_stats") return MessageType::QUERY_STATS;
    if (str == "query_history") return MessageType::QUERY_HISTORY;
    if (str == "response") return MessageType::RESPONSE;
    if (str == "error") return MessageType::ERROR;
    return MessageType::UNKNOWN;
}

// 序列化消息
std::string Message::serialize() const {
    json::object obj;

    obj["message_id"] = message_id;
    obj["type"] = type_to_string(type);
    obj["timestamp"] = timestamp;
    obj["priority"] = static_cast<int>(priority);

    if (!from_user.empty()) obj["from_user"] = from_user;
    if (!to_user.empty()) obj["to_user"] = to_user;
    if (!content.empty()) obj["content"] = content;
    if (!metadata.empty()) obj["metadata"] = metadata;
    if (!ack_id.empty()) obj["ack_id"] = ack_id;
    if (require_ack) obj["require_ack"] = true;

    return json::serialize(obj);
}

// 反序列化消息
std::optional<Message> Message::deserialize(const std::string& data) {
    try {
        auto jv = json::parse(data);
        auto& obj = jv.as_object();

        Message msg;

        msg.message_id = std::string(obj["message_id"].as_string());
        msg.type = string_to_type(std::string(obj["type"].as_string()));
        msg.timestamp = obj["timestamp"].as_int64();
        msg.priority = static_cast<MessagePriority>(obj["priority"].as_int64());

        if (obj.contains("from_user"))
            msg.from_user = std::string(obj["from_user"].as_string());
        if (obj.contains("to_user"))
            msg.to_user = std::string(obj["to_user"].as_string());
        if (obj.contains("content"))
            msg.content = std::string(obj["content"].as_string());
        if (obj.contains("metadata"))
            msg.metadata = obj["metadata"].as_object();
        if (obj.contains("ack_id"))
            msg.ack_id = std::string(obj["ack_id"].as_string());
        if (obj.contains("require_ack"))
            msg.require_ack = obj["require_ack"].as_bool();

        return msg;

    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// ConnectMessage 实现
Message ConnectMessage::to_message() const {
    Message msg;
    msg.message_id = Message::generate_message_id();
    msg.type = MessageType::CONNECT;
    msg.timestamp = Message::current_timestamp();
    msg.priority = MessagePriority::HIGH;
    msg.from_user = user_id;
    msg.require_ack = true;

    msg.metadata["token"] = token;
    msg.metadata["client_info"] = client_info;

    return msg;
}

std::optional<ConnectMessage> ConnectMessage::from_message(const Message& msg) {
    if (msg.type != MessageType::CONNECT) {
        return std::nullopt;
    }

    ConnectMessage conn;
    conn.user_id = msg.from_user;

    if (msg.metadata.contains("token")) {
        conn.token = std::string(msg.metadata.at("token").as_string());
    }
    if (msg.metadata.contains("client_info")) {
        conn.client_info = msg.metadata.at("client_info").as_object();
    }

    return conn;
}

// HeartbeatMessage 实现
Message HeartbeatMessage::to_message() const {
    Message msg;
    msg.message_id = Message::generate_message_id();
    msg.type = MessageType::HEARTBEAT;
    msg.timestamp = Message::current_timestamp();
    msg.priority = MessagePriority::LOW;
    msg.require_ack = false;

    msg.metadata["client_timestamp"] = client_timestamp;

    return msg;
}

std::optional<HeartbeatMessage> HeartbeatMessage::from_message(const Message& msg) {
    if (msg.type != MessageType::HEARTBEAT) {
        return std::nullopt;
    }

    HeartbeatMessage hb;
    if (msg.metadata.contains("client_timestamp")) {
        hb.client_timestamp = msg.metadata.at("client_timestamp").as_int64();
    }

    return hb;
}

// ChatMessage 实现
Message ChatMessage::to_message(const std::string& from_user) const {
    Message msg;
    msg.message_id = Message::generate_message_id();
    msg.type = MessageType::MESSAGE;
    msg.timestamp = Message::current_timestamp();
    msg.priority = priority;
    msg.from_user = from_user;
    msg.to_user = to_user;
    msg.content = content;
    msg.require_ack = require_ack;

    return msg;
}

std::optional<ChatMessage> ChatMessage::from_message(const Message& msg) {
    if (msg.type != MessageType::MESSAGE) {
        return std::nullopt;
    }

    ChatMessage chat;
    chat.to_user = msg.to_user;
    chat.content = msg.content;
    chat.priority = msg.priority;
    chat.require_ack = msg.require_ack;

    return chat;
}

// AckMessage 实现
Message AckMessage::to_message() const {
    Message msg;
    msg.message_id = Message::generate_message_id();
    msg.type = success ? MessageType::ACK : MessageType::NACK;
    msg.timestamp = Message::current_timestamp();
    msg.priority = MessagePriority::HIGH;
    msg.ack_id = ack_message_id;
    msg.require_ack = false;

    if (!success && !error_message.empty()) {
        msg.content = error_message;
    }

    return msg;
}

std::optional<AckMessage> AckMessage::from_message(const Message& msg) {
    if (msg.type != MessageType::ACK && msg.type != MessageType::NACK) {
        return std::nullopt;
    }

    AckMessage ack;
    ack.ack_message_id = msg.ack_id;
    ack.success = (msg.type == MessageType::ACK);
    ack.error_message = msg.content;

    return ack;
}

// ResponseMessage 实现
Message ResponseMessage::to_message() const {
    Message msg;
    msg.message_id = Message::generate_message_id();
    msg.type = success ? MessageType::RESPONSE : MessageType::ERROR;
    msg.timestamp = Message::current_timestamp();
    msg.priority = MessagePriority::NORMAL;
    msg.content = message;
    msg.metadata = data;
    msg.require_ack = false;

    return msg;
}

std::optional<ResponseMessage> ResponseMessage::from_message(const Message& msg) {
    if (msg.type != MessageType::RESPONSE && msg.type != MessageType::ERROR) {
        return std::nullopt;
    }

    ResponseMessage resp;
    resp.success = (msg.type == MessageType::RESPONSE);
    resp.message = msg.content;
    resp.data = msg.metadata;

    return resp;
}

}
