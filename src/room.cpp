/**
 * @file room.cpp
 * @brief 房间/频道管理实现
 */

#include "room.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace gateway {

// Room 实现
Room::Room(const std::string& room_id,
           const std::string& name,
           const std::string& owner_id,
           RoomType type) {
    info_.room_id = room_id;
    info_.name = name;
    info_.type = type;
    info_.owner_id = owner_id;
    info_.max_members = 100;
    info_.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // 添加所有者
    add_member(owner_id, MemberRole::OWNER);
}

bool Room::add_member(const std::string& user_id, MemberRole role) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (members_.size() >= static_cast<size_t>(info_.max_members)) {
        return false;
    }

    if (members_.find(user_id) != members_.end()) {
        return false;  // 已存在
    }

    RoomMember member(user_id, role);
    member.joined_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    members_[user_id] = member;
    return true;
}

bool Room::remove_member(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 不能删除所有者
    if (user_id == info_.owner_id) {
        return false;
    }

    return members_.erase(user_id) > 0;
}

bool Room::has_member(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return members_.find(user_id) != members_.end();
}

std::vector<RoomMember> Room::get_members() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<RoomMember> result;
    for (const auto& [uid, member] : members_) {
        result.push_back(member);
    }
    return result;
}

int Room::member_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return members_.size();
}

bool Room::set_role(const std::string& user_id, MemberRole role) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = members_.find(user_id);
    if (it == members_.end()) {
        return false;
    }

    // 不能修改所有者角色
    if (user_id == info_.owner_id) {
        return false;
    }

    it->second.role = role;
    return true;
}

MemberRole Room::get_role(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = members_.find(user_id);
    if (it == members_.end()) {
        return MemberRole::GUEST;
    }

    return it->second.role;
}

bool Room::can_send(const std::string& user_id) const {
    auto role = get_role(user_id);

    if (is_muted(user_id)) {
        return false;
    }

    // 访客不能发送
    return role != MemberRole::GUEST;
}

bool Room::can_invite(const std::string& user_id) const {
    auto role = get_role(user_id);
    // 管理员和所有者可以邀请
    return role == MemberRole::OWNER || role == MemberRole::ADMIN;
}

bool Room::can_kick(const std::string& user_id) const {
    auto role = get_role(user_id);
    // 管理员和所有者可以踢人
    return role == MemberRole::OWNER || role == MemberRole::ADMIN;
}

bool Room::mute_member(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = members_.find(user_id);
    if (it == members_.end()) {
        return false;
    }

    // 不能静音所有者
    if (user_id == info_.owner_id) {
        return false;
    }

    it->second.muted = true;
    return true;
}

bool Room::unmute_member(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = members_.find(user_id);
    if (it == members_.end()) {
        return false;
    }

    it->second.muted = false;
    return true;
}

bool Room::is_muted(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = members_.find(user_id);
    if (it == members_.end()) {
        return false;
    }

    return it->second.muted;
}

void Room::set_max_members(int max) {
    std::lock_guard<std::mutex> lock(mutex_);
    info_.max_members = max;
}

bool Room::is_full() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return members_.size() >= static_cast<size_t>(info_.max_members);
}

RoomInfo Room::get_info() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

// RoomManager 实现
RoomManager& RoomManager::instance() {
    static RoomManager manager;
    return manager;
}

std::string RoomManager::generate_room_id() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    uint64_t id = dis(gen);

    std::ostringstream oss;
    oss << "room_" << std::hex << std::setfill('0') << std::setw(16) << id;
    return oss.str();
}

std::string RoomManager::create_room(const std::string& name,
                                     const std::string& owner_id,
                                     RoomType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string room_id = generate_room_id();

    auto room = std::make_shared<Room>(room_id, name, owner_id, type);
    rooms_[room_id] = room;
    user_rooms_[owner_id].insert(room_id);

    return room_id;
}

bool RoomManager::delete_room(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) {
        return false;
    }

    // 从用户房间列表中删除
    auto members = it->second->get_members();
    for (const auto& member : members) {
        user_rooms_[member.user_id].erase(room_id);
    }

    rooms_.erase(it);
    return true;
}

std::shared_ptr<Room> RoomManager::get_room(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) {
        return nullptr;
    }

    return it->second;
}

std::vector<RoomInfo> RoomManager::list_rooms(RoomType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<RoomInfo> result;

    for (const auto& [room_id, room] : rooms_) {
        if (room->type() == type || type == RoomType::GROUP) {
            result.push_back(room->get_info());
        }
    }

    return result;
}

bool RoomManager::join_room(const std::string& room_id, const std::string& user_id) {
    auto room = get_room(room_id);
    if (!room) {
        return false;
    }

    if (room->is_full()) {
        return false;
    }

    if (!room->add_member(user_id)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    user_rooms_[user_id].insert(room_id);

    return true;
}

bool RoomManager::leave_room(const std::string& room_id, const std::string& user_id) {
    auto room = get_room(room_id);
    if (!room) {
        return false;
    }

    if (!room->remove_member(user_id)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    user_rooms_[user_id].erase(room_id);

    return true;
}

bool RoomManager::kick_member(const std::string& room_id,
                              const std::string& user_id,
                              const std::string& operator_id) {
    auto room = get_room(room_id);
    if (!room) {
        return false;
    }

    // 检查操作者权限
    if (!room->can_kick(operator_id)) {
        return false;
    }

    return leave_room(room_id, user_id);
}

std::vector<std::string> RoomManager::get_user_rooms(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> result;

    auto it = user_rooms_.find(user_id);
    if (it != user_rooms_.end()) {
        for (const auto& room_id : it->second) {
            result.push_back(room_id);
        }
    }

    return result;
}

std::vector<RoomMember> RoomManager::get_room_members(const std::string& room_id) {
    auto room = get_room(room_id);
    if (!room) {
        return {};
    }

    return room->get_members();
}

std::vector<std::string> RoomManager::get_broadcast_targets(const std::string& room_id,
                                                            const std::string& sender_id) {
    auto room = get_room(room_id);
    if (!room) {
        return {};
    }

    // 检查发送者权限
    if (!room->can_send(sender_id)) {
        return {};
    }

    // 获取所有成员（除了发送者自己）
    auto members = room->get_members();
    std::vector<std::string> targets;

    for (const auto& member : members) {
        if (member.user_id != sender_id) {
            targets.push_back(member.user_id);
        }
    }

    return targets;
}

void RoomManager::cleanup_empty_rooms() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> to_delete;

    for (const auto& [room_id, room] : rooms_) {
        // 只清理临时房间（PRIVATE）
        if (room->type() == RoomType::PRIVATE && room->member_count() == 0) {
            to_delete.push_back(room_id);
        }
    }

    for (const auto& room_id : to_delete) {
        rooms_.erase(room_id);
    }
}

}
