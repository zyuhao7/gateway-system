/**
 * @file test_message_storage.cpp
 * @brief 消息存储测试
 */

#include "message_storage.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>

using namespace gateway;

int main() {
    std::cout << "=== Message Storage Test ===" << std::endl;

    // 测试数据库路径
    std::string db_path = "/tmp/test_message_storage";

    // 清理旧数据
    if (std::filesystem::exists(db_path)) {
        std::filesystem::remove_all(db_path);
    }

    auto& storage = MessageStorage::instance();

    // 测试 1: 打开数据库
    std::cout << "\n[Test 1] Opening database..." << std::endl;
    if (!storage.open(db_path)) {
        std::cerr << "Failed to open database" << std::endl;
        return 1;
    }
    std::cout << "✓ Database opened" << std::endl;

    // 测试 2: 存储消息
    std::cout << "\n[Test 2] Storing messages..." << std::endl;
    auto now = std::chrono::system_clock::now().time_since_epoch().count();

    StoredMessage msg1;
    msg1.message_id = "msg001";
    msg1.from_user = "user_alice";
    msg1.to_user = "user_bob";
    msg1.content = "Hello Bob!";
    msg1.timestamp = now;
    msg1.delivered = false;

    StoredMessage msg2;
    msg2.message_id = "msg002";
    msg2.from_user = "user_charlie";
    msg2.to_user = "user_bob";
    msg2.content = "Hi Bob, how are you?";
    msg2.timestamp = now + 1000;
    msg2.delivered = false;

    if (!storage.store_message(msg1) || !storage.store_message(msg2)) {
        std::cerr << "Failed to store messages" << std::endl;
        return 1;
    }
    std::cout << "✓ Stored 2 messages" << std::endl;

    // 测试 3: 获取离线消息
    std::cout << "\n[Test 3] Getting offline messages..." << std::endl;
    auto offline_msgs = storage.get_offline_messages("user_bob");
    std::cout << "Found " << offline_msgs.size() << " offline messages" << std::endl;

    for (const auto& msg : offline_msgs) {
        std::cout << "  - From: " << msg.from_user
                  << ", Content: " << msg.content
                  << ", Delivered: " << (msg.delivered ? "yes" : "no") << std::endl;
    }

    if (offline_msgs.size() != 2) {
        std::cerr << "Expected 2 offline messages, got " << offline_msgs.size() << std::endl;
        return 1;
    }
    std::cout << "✓ Offline messages retrieved" << std::endl;

    // 测试 4: 标记为已送达
    std::cout << "\n[Test 4] Marking message as delivered..." << std::endl;
    if (!storage.mark_delivered("msg001")) {
        std::cerr << "Failed to mark message as delivered" << std::endl;
        return 1;
    }
    std::cout << "✓ Message marked as delivered" << std::endl;

    // 验证已送达消息不再出现在离线消息中
    offline_msgs = storage.get_offline_messages("user_bob");
    std::cout << "Offline messages after delivery: " << offline_msgs.size() << std::endl;

    if (offline_msgs.size() != 1) {
        std::cerr << "Expected 1 offline message, got " << offline_msgs.size() << std::endl;
        return 1;
    }

    // 测试 5: 历史消息查询
    std::cout << "\n[Test 5] Querying message history..." << std::endl;
    auto history = storage.get_history("user_bob", now - 1000, now + 2000, 10);
    std::cout << "Found " << history.size() << " messages in history" << std::endl;

    for (const auto& msg : history) {
        std::cout << "  - " << msg.message_id << ": " << msg.content << std::endl;
    }

    if (history.size() != 2) {
        std::cerr << "Expected 2 messages in history, got " << history.size() << std::endl;
        return 1;
    }
    std::cout << "✓ History query successful" << std::endl;

    // 测试 6: 清理旧消息
    std::cout << "\n[Test 6] Cleaning up old messages..." << std::endl;
    size_t deleted = storage.cleanup_old_messages(now + 500);
    std::cout << "Deleted " << deleted << " old messages" << std::endl;

    if (deleted != 1) {
        std::cerr << "Expected to delete 1 message, deleted " << deleted << std::endl;
        return 1;
    }
    std::cout << "✓ Old messages cleaned up" << std::endl;

    // 关闭数据库
    std::cout << "\n[Test 7] Closing database..." << std::endl;
    storage.close();
    std::cout << "✓ Database closed" << std::endl;

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
