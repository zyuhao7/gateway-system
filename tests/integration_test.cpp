/**
 * @file integration_test.cpp
 * @brief 端到端集成测试
 *
 * 测试场景：
 * 1. WebSocket连接 + JWT认证
 * 2. 创建房间 + 加入房间
 * 3. 发送群组消息 + 速率限制
 * 4. 离线消息存储 + 历史查询
 */

#include "auth.hpp"
#include "rate_limiter.hpp"
#include "room.hpp"
#include "protocol.hpp"
#include "message_storage.hpp"
#include <iostream>
#include <cassert>

using namespace gateway;

// 模拟场景：完整的IM工作流
void test_complete_im_workflow() {
    std::cout << "=== Integration Test: Complete IM Workflow ===" << std::endl;
    std::cout << std::endl;

    // ========== 步骤1: 用户认证 ==========
    std::cout << "[Step 1] User Authentication" << std::endl;

    AuthManager::instance().set_secret("integration_test_secret");

    // 生成Token
    std::string alice_token = AuthManager::instance().generate_token("alice");
    std::string bob_token = AuthManager::instance().generate_token("bob");
    std::string charlie_token = AuthManager::instance().generate_token("charlie");

    std::cout << "  Alice token: " << alice_token.substr(0, 30) << "..." << std::endl;
    std::cout << "  Bob token: " << bob_token.substr(0, 30) << "..." << std::endl;

    // 验证Token
    auto alice_verified = AuthManager::instance().verify_token(alice_token);
    auto bob_verified = AuthManager::instance().verify_token(bob_token);

    assert(alice_verified.has_value());
    assert(bob_verified.has_value());
    assert(*alice_verified == "alice");
    assert(*bob_verified == "bob");

    std::cout << "  ✓ All users authenticated" << std::endl;
    std::cout << std::endl;

    // ========== 步骤2: 速率限制检查 ==========
    std::cout << "[Step 2] Rate Limiting" << std::endl;

    IPRateLimiter::instance().configure(10, 20);
    UserRateLimiter::instance().configure(100, 150);

    // 检查IP限流
    auto ip_result = IPRateLimiter::instance().check("192.168.1.100");
    assert(ip_result.allowed);
    std::cout << "  IP rate limit: OK (remaining: " << ip_result.remaining << ")" << std::endl;

    // 检查用户限流
    auto user_result = UserRateLimiter::instance().check("alice");
    assert(user_result.allowed);
    std::cout << "  User rate limit: OK (remaining: " << user_result.remaining << ")" << std::endl;

    std::cout << "  ✓ Rate limiting passed" << std::endl;
    std::cout << std::endl;

    // ========== 步骤3: 创建房间 ==========
    std::cout << "[Step 3] Room Creation" << std::endl;

    auto& room_mgr = RoomManager::instance();

    std::string room_id = room_mgr.create_room("Dev Team Chat", "alice", RoomType::GROUP);
    std::cout << "  Created room: " << room_id << std::endl;

    auto room = room_mgr.get_room(room_id);
    assert(room != nullptr);
    assert(room->name() == "Dev Team Chat");
    assert(room->owner_id() == "alice");

    std::cout << "  ✓ Room created successfully" << std::endl;
    std::cout << std::endl;

    // ========== 步骤4: 成员加入 ==========
    std::cout << "[Step 4] Members Joining" << std::endl;

    assert(room_mgr.join_room(room_id, "bob"));
    assert(room_mgr.join_room(room_id, "charlie"));

    std::cout << "  Bob joined" << std::endl;
    std::cout << "  Charlie joined" << std::endl;
    std::cout << "  Total members: " << room->member_count() << std::endl;

    assert(room->member_count() == 3);

    std::cout << "  ✓ All members joined" << std::endl;
    std::cout << std::endl;

    // ========== 步骤5: 权限管理 ==========
    std::cout << "[Step 5] Permission Management" << std::endl;

    // 设置Bob为管理员
    assert(room->set_role("bob", MemberRole::ADMIN));
    assert(room->get_role("bob") == MemberRole::ADMIN);

    std::cout << "  Bob promoted to ADMIN" << std::endl;

    // 检查权限
    assert(room->can_send("alice"));
    assert(room->can_send("bob"));
    assert(room->can_send("charlie"));
    assert(room->can_invite("alice"));
    assert(room->can_invite("bob"));
    assert(!room->can_invite("charlie"));

    std::cout << "  ✓ Permissions configured" << std::endl;
    std::cout << std::endl;

    // ========== 步骤6: 发送群组消息 ==========
    std::cout << "[Step 6] Sending Group Messages" << std::endl;

    // Alice发送消息
    ChatMessage msg1;
    msg1.to_user = room_id;  // 房间ID
    msg1.content = "Hello team!";
    msg1.priority = MessagePriority::NORMAL;

    Message proto_msg1 = msg1.to_message("alice");
    std::cout << "  Alice: " << msg1.content << std::endl;
    std::cout << "  Message ID: " << proto_msg1.message_id << std::endl;

    // 获取广播目标
    auto targets = room_mgr.get_broadcast_targets(room_id, "alice");
    assert(targets.size() == 2);  // bob和charlie
    std::cout << "  Broadcast to: " << targets.size() << " members" << std::endl;

    std::cout << "  ✓ Message sent" << std::endl;
    std::cout << std::endl;

    // ========== 步骤7: 消息持久化 ==========
    std::cout << "[Step 7] Message Persistence" << std::endl;

    auto& storage = MessageStorage::instance();
    storage.open("/tmp/integration_test_messages");

    // 存储消息
    StoredMessage stored1;
    stored1.message_id = proto_msg1.message_id;
    stored1.from_user = "alice";
    stored1.to_user = "bob";
    stored1.content = msg1.content;
    stored1.timestamp = proto_msg1.timestamp;
    stored1.delivered = false;

    assert(storage.store_message(stored1));
    std::cout << "  Message stored for bob" << std::endl;

    // 模拟bob离线，再存一条
    StoredMessage stored2;
    stored2.message_id = Message::generate_message_id();
    stored2.from_user = "alice";
    stored2.to_user = "bob";
    stored2.content = "Are you there?";
    stored2.timestamp = Message::current_timestamp();
    stored2.delivered = false;

    assert(storage.store_message(stored2));
    std::cout << "  Another message stored for bob" << std::endl;

    // 查询bob的离线消息
    auto offline = storage.get_offline_messages("bob");
    assert(offline.size() == 2);
    std::cout << "  Bob has " << offline.size() << " offline messages" << std::endl;

    std::cout << "  ✓ Messages persisted" << std::endl;
    std::cout << std::endl;

    // ========== 步骤8: 用户上线接收离线消息 ==========
    std::cout << "[Step 8] User Online - Receive Offline Messages" << std::endl;

    std::cout << "  Bob comes online..." << std::endl;

    for (const auto& msg : offline) {
        std::cout << "    From " << msg.from_user << ": " << msg.content << std::endl;

        // 标记为已送达
        storage.mark_delivered(msg.message_id);
    }

    // 检查离线消息是否清空
    auto remaining = storage.get_offline_messages("bob");
    assert(remaining.size() == 0);
    std::cout << "  ✓ All offline messages delivered" << std::endl;
    std::cout << std::endl;

    // ========== 步骤9: 历史消息查询 ==========
    std::cout << "[Step 9] Query Message History" << std::endl;

    int64_t start_time = proto_msg1.timestamp - 1000;
    int64_t end_time = Message::current_timestamp();

    auto history = storage.get_history("bob", start_time, end_time, 10);
    std::cout << "  Found " << history.size() << " messages in history" << std::endl;

    for (const auto& msg : history) {
        std::cout << "    [" << msg.timestamp << "] "
                  << msg.from_user << ": " << msg.content << std::endl;
    }

    std::cout << "  ✓ History retrieved" << std::endl;
    std::cout << std::endl;

    // ========== 步骤10: 静音功能 ==========
    std::cout << "[Step 10] Mute Feature" << std::endl;

    // 管理员Bob静音Charlie
    assert(room->mute_member("charlie"));
    assert(room->is_muted("charlie"));
    std::cout << "  Charlie muted by Bob" << std::endl;

    // Charlie不能发送消息
    assert(!room->can_send("charlie"));
    std::cout << "  Charlie cannot send messages" << std::endl;

    // 解除静音
    assert(room->unmute_member("charlie"));
    assert(room->can_send("charlie"));
    std::cout << "  Charlie unmuted" << std::endl;

    std::cout << "  ✓ Mute feature working" << std::endl;
    std::cout << std::endl;

    // ========== 步骤11: 成员离开 ==========
    std::cout << "[Step 11] Member Leaving" << std::endl;

    assert(room_mgr.leave_room(room_id, "charlie"));
    assert(room->member_count() == 2);
    std::cout << "  Charlie left the room" << std::endl;
    std::cout << "  Remaining members: " << room->member_count() << std::endl;

    std::cout << "  ✓ Member left successfully" << std::endl;
    std::cout << std::endl;

    // ========== 步骤12: 清理 ==========
    std::cout << "[Step 12] Cleanup" << std::endl;

    // 删除房间
    assert(room_mgr.delete_room(room_id));
    assert(room_mgr.get_room(room_id) == nullptr);
    std::cout << "  Room deleted" << std::endl;

    // 关闭存储
    storage.close();
    std::cout << "  Storage closed" << std::endl;

    std::cout << "  ✓ Cleanup completed" << std::endl;
    std::cout << std::endl;

    std::cout << "=== Integration Test: PASSED ===" << std::endl;
}

int main() {
    try {
        test_complete_im_workflow();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
