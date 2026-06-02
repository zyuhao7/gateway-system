/**
 * @file room.hpp
 * @brief 房间/频道管理模块
 */

#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <memory>
#include <optional>
#include <chrono>

namespace gateway {

// 房间类型
enum class RoomType {
    PRIVATE,        // 私密房间（邀请制）
    PUBLIC,         // 公开房间（任何人可加入）
    GROUP,          // 群组（持久化）
    CHANNEL         // 频道（广播）
};

// 用户角色
enum class MemberRole {
    OWNER,          // 所有者
    ADMIN,          // 管理员
    MEMBER,         // 普通成员
    GUEST           // 访客（只读）
};

// 房间成员
struct RoomMember {
    std::string user_id;
    MemberRole role;
    int64_t joined_at;
    bool muted;

    RoomMember() : role(MemberRole::MEMBER), joined_at(0), muted(false) {}
    RoomMember(const std::string& uid, MemberRole r)
        : user_id(uid), role(r), joined_at(0), muted(false) {}
};

// 房间信息
struct RoomInfo {
    std::string room_id;
    std::string name;
    std::string description;
    RoomType type;
    std::string owner_id;
    int max_members;
    int64_t created_at;
    std::map<std::string, std::string> metadata;
};

// 房间
class Room {
public:
    Room(const std::string& room_id,
         const std::string& name,
         const std::string& owner_id,
         RoomType type = RoomType::GROUP);

    // 基本信息
    std::string id() const { return info_.room_id; }
    std::string name() const { return info_.name; }
    RoomType type() const { return info_.type; }
    std::string owner_id() const { return info_.owner_id; }

    // 成员管理
    bool add_member(const std::string& user_id, MemberRole role = MemberRole::MEMBER);
    bool remove_member(const std::string& user_id);
    bool has_member(const std::string& user_id) const;
    std::vector<RoomMember> get_members() const;
    int member_count() const;

    // 权限管理
    bool set_role(const std::string& user_id, MemberRole role);
    MemberRole get_role(const std::string& user_id) const;
    bool can_send(const std::string& user_id) const;
    bool can_invite(const std::string& user_id) const;
    bool can_kick(const std::string& user_id) const;

    // 静音管理
    bool mute_member(const std::string& user_id);
    bool unmute_member(const std::string& user_id);
    bool is_muted(const std::string& user_id) const;

    // 房间设置
    void set_max_members(int max);
    int max_members() const { return info_.max_members; }
    bool is_full() const;

    // 获取房间信息
    RoomInfo get_info() const;

private:
    RoomInfo info_;
    std::map<std::string, RoomMember> members_;
    mutable std::mutex mutex_;
};

// 房间管理器
class RoomManager {
public:
    static RoomManager& instance();

    // 房间CRUD
    std::string create_room(const std::string& name,
                           const std::string& owner_id,
                           RoomType type = RoomType::GROUP);
    bool delete_room(const std::string& room_id);
    std::shared_ptr<Room> get_room(const std::string& room_id);
    std::vector<RoomInfo> list_rooms(RoomType type = RoomType::GROUP);

    // 成员操作
    bool join_room(const std::string& room_id, const std::string& user_id);
    bool leave_room(const std::string& room_id, const std::string& user_id);
    bool kick_member(const std::string& room_id,
                    const std::string& user_id,
                    const std::string& operator_id);

    // 查询
    std::vector<std::string> get_user_rooms(const std::string& user_id);
    std::vector<RoomMember> get_room_members(const std::string& room_id);

    // 消息广播
    std::vector<std::string> get_broadcast_targets(const std::string& room_id,
                                                   const std::string& sender_id);

    // 清理
    void cleanup_empty_rooms();

private:
    RoomManager() = default;

    std::map<std::string, std::shared_ptr<Room>> rooms_;
    std::map<std::string, std::set<std::string>> user_rooms_;  // user -> rooms
    mutable std::mutex mutex_;

    std::string generate_room_id();
};

}
