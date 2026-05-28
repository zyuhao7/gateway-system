/**
 * @file message_storage.hpp
 * @brief 消息持久化存储
 */

#pragma once

#include <rocksdb/db.h>
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace gateway {

struct StoredMessage {
    std::string message_id;
    std::string from_user;
    std::string to_user;
    std::string content;
    int64_t timestamp;
    bool delivered;

    std::string serialize() const;
    static std::optional<StoredMessage> deserialize(const std::string& data);
};

class MessageStorage {
public:
    static MessageStorage& instance();

    bool open(const std::string& db_path);
    void close();

    // 存储消息
    bool store_message(const StoredMessage& msg);

    // 获取用户的离线消息
    std::vector<StoredMessage> get_offline_messages(const std::string& user_id);

    // 标记消息为已送达
    bool mark_delivered(const std::string& message_id);

    // 获取历史消息
    std::vector<StoredMessage> get_history(const std::string& user_id,
                                           int64_t start_time,
                                           int64_t end_time,
                                           size_t limit = 100);

    // 删除旧消息
    size_t cleanup_old_messages(int64_t before_timestamp);

private:
    MessageStorage() = default;
    ~MessageStorage();

    MessageStorage(const MessageStorage&) = delete;
    MessageStorage& operator=(const MessageStorage&) = delete;

    std::unique_ptr<rocksdb::DB> db_;
    bool is_open_ = false;

    std::string make_message_key(const std::string& message_id);
    std::string make_user_message_key(const std::string& user_id, int64_t timestamp);
};

}
