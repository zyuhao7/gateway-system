/**
 * @file test_room.cpp
 * @brief 房间/频道测试
 */

#include "room.hpp"
#include <iostream>
#include <cassert>

using namespace gateway;

void test_room_creation() {
    std::cout << "[Test 1] Room creation..." << std::endl;

    Room room("room_001", "Test Room", "alice", RoomType::GROUP);

    assert(room.id() == "room_001");
    assert(room.name() == "Test Room");
    assert(room.owner_id() == "alice");
    assert(room.member_count() == 1);  // Owner auto-added

    std::cout << "  Room: " << room.name() << std::endl;
    std::cout << "  Owner: " << room.owner_id() << std::endl;
    std::cout << "  Members: " << room.member_count() << std::endl;

    std::cout << "✓ Room creation passed" << std::endl;
}

void test_member_management() {
    std::cout << "\n[Test 2] Member management..." << std::endl;

    Room room("room_002", "Chat Room", "alice", RoomType::GROUP);

    // 添加成员
    assert(room.add_member("bob"));
    assert(room.add_member("charlie"));
    assert(room.member_count() == 3);

    // 重复添加
    assert(!room.add_member("bob"));

    // 检查成员
    assert(room.has_member("alice"));
    assert(room.has_member("bob"));
    assert(!room.has_member("dave"));

    // 删除成员
    assert(room.remove_member("bob"));
    assert(!room.has_member("bob"));
    assert(room.member_count() == 2);

    // 不能删除所有者
    assert(!room.remove_member("alice"));

    std::cout << "  Final members: " << room.member_count() << std::endl;
    std::cout << "✓ Member management passed" << std::endl;
}

void test_role_management() {
    std::cout << "\n[Test 3] Role management..." << std::endl;

    Room room("room_003", "Admin Room", "alice", RoomType::GROUP);
    room.add_member("bob");
    room.add_member("charlie");

    // 检查角色
    assert(room.get_role("alice") == MemberRole::OWNER);
    assert(room.get_role("bob") == MemberRole::MEMBER);

    // 设置管理员
    assert(room.set_role("bob", MemberRole::ADMIN));
    assert(room.get_role("bob") == MemberRole::ADMIN);

    // 权限检查
    assert(room.can_invite("alice"));      // Owner can invite
    assert(room.can_invite("bob"));        // Admin can invite
    assert(!room.can_invite("charlie"));   // Member cannot invite

    assert(room.can_kick("alice"));        // Owner can kick
    assert(room.can_kick("bob"));          // Admin can kick
    assert(!room.can_kick("charlie"));     // Member cannot kick

    std::cout << "  Alice: OWNER" << std::endl;
    std::cout << "  Bob: ADMIN" << std::endl;
    std::cout << "  Charlie: MEMBER" << std::endl;

    std::cout << "✓ Role management passed" << std::endl;
}

void test_mute_feature() {
    std::cout << "\n[Test 4] Mute feature..." << std::endl;

    Room room("room_004", "Moderated Room", "alice", RoomType::GROUP);
    room.add_member("bob");

    // 正常可以发送
    assert(room.can_send("bob"));

    // 静音
    assert(room.mute_member("bob"));
    assert(room.is_muted("bob"));
    assert(!room.can_send("bob"));

    // 解除静音
    assert(room.unmute_member("bob"));
    assert(!room.is_muted("bob"));
    assert(room.can_send("bob"));

    // 不能静音所有者
    assert(!room.mute_member("alice"));

    std::cout << "✓ Mute feature passed" << std::endl;
}

void test_room_capacity() {
    std::cout << "\n[Test 5] Room capacity..." << std::endl;

    Room room("room_005", "Small Room", "alice", RoomType::GROUP);
    room.set_max_members(3);

    assert(room.add_member("bob"));
    assert(room.add_member("charlie"));
    assert(room.is_full());

    // 满了不能加入
    assert(!room.add_member("dave"));

    std::cout << "  Max members: " << room.max_members() << std::endl;
    std::cout << "  Current: " << room.member_count() << std::endl;

    std::cout << "✓ Room capacity passed" << std::endl;
}

void test_room_manager() {
    std::cout << "\n[Test 6] Room manager..." << std::endl;

    auto& manager = RoomManager::instance();

    // 创建房间
    std::string room_id = manager.create_room("Test Room", "alice", RoomType::GROUP);
    assert(!room_id.empty());

    std::cout << "  Created room: " << room_id << std::endl;

    // 获取房间
    auto room = manager.get_room(room_id);
    assert(room != nullptr);
    assert(room->name() == "Test Room");

    // 加入房间
    assert(manager.join_room(room_id, "bob"));
    assert(manager.join_room(room_id, "charlie"));
    assert(room->member_count() == 3);

    // 离开房间
    assert(manager.leave_room(room_id, "bob"));
    assert(room->member_count() == 2);

    // 删除房间
    assert(manager.delete_room(room_id));
    assert(manager.get_room(room_id) == nullptr);

    std::cout << "✓ Room manager passed" << std::endl;
}

void test_broadcast_targets() {
    std::cout << "\n[Test 7] Broadcast targets..." << std::endl;

    auto& manager = RoomManager::instance();

    std::string room_id = manager.create_room("Broadcast Room", "alice");
    manager.join_room(room_id, "bob");
    manager.join_room(room_id, "charlie");
    manager.join_room(room_id, "dave");

    // 获取广播目标（不包括发送者自己）
    auto targets = manager.get_broadcast_targets(room_id, "alice");
    assert(targets.size() == 3);

    std::cout << "  Sender: alice" << std::endl;
    std::cout << "  Targets: " << targets.size() << std::endl;
    for (const auto& target : targets) {
        std::cout << "    - " << target << std::endl;
    }

    // 静音用户不影响广播列表（在实际发送时过滤）
    auto room = manager.get_room(room_id);
    room->mute_member("bob");

    targets = manager.get_broadcast_targets(room_id, "alice");
    assert(targets.size() == 3);  // 仍然包含bob（在发送时检查）

    manager.delete_room(room_id);

    std::cout << "✓ Broadcast targets passed" << std::endl;
}

void test_user_rooms() {
    std::cout << "\n[Test 8] User rooms..." << std::endl;

    auto& manager = RoomManager::instance();

    std::string room1 = manager.create_room("Room 1", "alice");
    std::string room2 = manager.create_room("Room 2", "bob");

    manager.join_room(room1, "bob");
    manager.join_room(room2, "alice");

    // Alice在2个房间
    auto alice_rooms = manager.get_user_rooms("alice");
    assert(alice_rooms.size() == 2);

    // Bob也在2个房间
    auto bob_rooms = manager.get_user_rooms("bob");
    assert(bob_rooms.size() == 2);

    std::cout << "  Alice's rooms: " << alice_rooms.size() << std::endl;
    std::cout << "  Bob's rooms: " << bob_rooms.size() << std::endl;

    manager.delete_room(room1);
    manager.delete_room(room2);

    std::cout << "✓ User rooms passed" << std::endl;
}

int main() {
    std::cout << "=== Room Management Test ===" << std::endl;
    std::cout << std::endl;

    test_room_creation();
    test_member_management();
    test_role_management();
    test_mute_feature();
    test_room_capacity();
    test_room_manager();
    test_broadcast_targets();
    test_user_rooms();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
