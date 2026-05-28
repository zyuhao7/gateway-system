/**
 * @file message_storage.cpp
 * @brief 消息持久化存储实现
 */

#include "message_storage.hpp"
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>

namespace gateway {

namespace json = boost::json;

// StoredMessage 序列化
std::string StoredMessage::serialize() const {
    json::object obj;
    obj["message_id"] = message_id;
    obj["from_user"] = from_user;
    obj["to_user"] = to_user;
    obj["content"] = content;
    obj["timestamp"] = timestamp;
    obj["delivered"] = delivered;
    return json::serialize(obj);
}

// StoredMessage 反序列化
std::optional<StoredMessage> StoredMessage::deserialize(const std::string& data) {
    try {
        auto jv = json::parse(data);
        auto& obj = jv.as_object();

        StoredMessage msg;
        msg.message_id = std::string(obj["message_id"].as_string());
        msg.from_user = std::string(obj["from_user"].as_string());
        msg.to_user = std::string(obj["to_user"].as_string());
        msg.content = std::string(obj["content"].as_string());
        msg.timestamp = obj["timestamp"].as_int64();
        msg.delivered = obj["delivered"].as_bool();

        return msg;
    } catch (const std::exception& e) {
        std::cerr << "Failed to deserialize message: " << e.what() << std::endl;
        return std::nullopt;
    }
}

MessageStorage& MessageStorage::instance() {
    static MessageStorage storage;
    return storage;
}

MessageStorage::~MessageStorage() {
    close();
}

bool MessageStorage::open(const std::string& db_path) {
    if (is_open_) {
        return true;
    }

    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kSnappyCompression;

    rocksdb::DB* db_ptr;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_ptr);

    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
        return false;
    }

    db_.reset(db_ptr);
    is_open_ = true;

    std::cout << "MessageStorage opened: " << db_path << std::endl;
    return true;
}

void MessageStorage::close() {
    if (is_open_) {
        db_.reset();
        is_open_ = false;
        std::cout << "MessageStorage closed" << std::endl;
    }
}

std::string MessageStorage::make_message_key(const std::string& message_id) {
    return "msg:" + message_id;
}

std::string MessageStorage::make_user_message_key(const std::string& user_id, int64_t timestamp) {
    std::ostringstream oss;
    oss << "user:" << user_id << ":" << timestamp;
    return oss.str();
}

bool MessageStorage::store_message(const StoredMessage& msg) {
    if (!is_open_) {
        return false;
    }

    std::string data = msg.serialize();

    // 存储消息本身
    std::string msg_key = make_message_key(msg.message_id);
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), msg_key, data);

    if (!status.ok()) {
        std::cerr << "Failed to store message: " << status.ToString() << std::endl;
        return false;
    }

    // 为接收者创建索引
    std::string user_key = make_user_message_key(msg.to_user, msg.timestamp);
    status = db_->Put(rocksdb::WriteOptions(), user_key, msg.message_id);

    if (!status.ok()) {
        std::cerr << "Failed to create user index: " << status.ToString() << std::endl;
        return false;
    }

    return true;
}

std::vector<StoredMessage> MessageStorage::get_offline_messages(const std::string& user_id) {
    std::vector<StoredMessage> messages;

    if (!is_open_) {
        return messages;
    }

    // 扫描用户的消息索引
    std::string prefix = "user:" + user_id + ":";

    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options));

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        std::string message_id = it->value().ToString();

        // 读取消息内容
        std::string msg_key = make_message_key(message_id);
        std::string data;
        rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), msg_key, &data);

        if (status.ok()) {
            auto msg_opt = StoredMessage::deserialize(data);
            if (msg_opt && !msg_opt->delivered) {
                messages.push_back(*msg_opt);
            }
        }
    }

    return messages;
}

bool MessageStorage::mark_delivered(const std::string& message_id) {
    if (!is_open_) {
        return false;
    }

    std::string msg_key = make_message_key(message_id);
    std::string data;

    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), msg_key, &data);
    if (!status.ok()) {
        return false;
    }

    auto msg_opt = StoredMessage::deserialize(data);
    if (!msg_opt) {
        return false;
    }

    msg_opt->delivered = true;
    std::string updated_data = msg_opt->serialize();

    status = db_->Put(rocksdb::WriteOptions(), msg_key, updated_data);
    return status.ok();
}

std::vector<StoredMessage> MessageStorage::get_history(const std::string& user_id,
                                                       int64_t start_time,
                                                       int64_t end_time,
                                                       size_t limit) {
    std::vector<StoredMessage> messages;

    if (!is_open_) {
        return messages;
    }

    std::string start_key = make_user_message_key(user_id, start_time);
    std::string end_key = make_user_message_key(user_id, end_time);

    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options));

    for (it->Seek(start_key); it->Valid() && it->key().ToString() <= end_key; it->Next()) {
        if (messages.size() >= limit) {
            break;
        }

        std::string message_id = it->value().ToString();
        std::string msg_key = make_message_key(message_id);
        std::string data;

        rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), msg_key, &data);
        if (status.ok()) {
            auto msg_opt = StoredMessage::deserialize(data);
            if (msg_opt) {
                messages.push_back(*msg_opt);
            }
        }
    }

    return messages;
}

size_t MessageStorage::cleanup_old_messages(int64_t before_timestamp) {
    if (!is_open_) {
        return 0;
    }

    size_t deleted_count = 0;

    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options));

    std::vector<std::string> keys_to_delete;

    // 扫描所有消息
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();

        if (key.starts_with("msg:")) {
            std::string data = it->value().ToString();
            auto msg_opt = StoredMessage::deserialize(data);

            if (msg_opt && msg_opt->timestamp < before_timestamp && msg_opt->delivered) {
                keys_to_delete.push_back(key);
            }
        }
    }

    // 删除旧消息
    for (const auto& key : keys_to_delete) {
        rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
        if (status.ok()) {
            deleted_count++;
        }
    }

    std::cout << "Cleaned up " << deleted_count << " old messages" << std::endl;
    return deleted_count;
}

}
