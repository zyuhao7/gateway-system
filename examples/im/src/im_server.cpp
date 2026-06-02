/**
 * @file im_server.cpp
 * @brief IM服务器示例 - 完整可运行的即时通讯服务器
 */

#include "websocket_server.hpp"
#include "auth.hpp"
#include "rate_limiter.hpp"
#include "room.hpp"
#include "protocol.hpp"
#include "message_storage.hpp"
#include <boost/json.hpp>
#include <iostream>
#include <map>
#include <memory>

using namespace gateway;
namespace json = boost::json;

class IMServer {
public:
    IMServer(uint16_t port) : port_(port) {
        // 初始化认证
        AuthManager::instance().set_secret("im_server_secret_key_2025");

        // 配置速率限制
        IPRateLimiter::instance().configure(100, 200);      // 100 req/s
        UserRateLimiter::instance().configure(1000, 1500);  // 1000 req/min

        // 打开消息存储
        MessageStorage::instance().open("./data/im_messages");

        std::cout << "IM Server initialized on port " << port_ << std::endl;
    }

    void start() {
        std::cout << "IM Server starting..." << std::endl;
        std::cout << "WebSocket endpoint: ws://localhost:" << port_ << std::endl;
        std::cout << std::endl;
        std::cout << "Available commands:" << std::endl;
        std::cout << "  CONNECT <token>        - Authenticate user" << std::endl;
        std::cout << "  CREATE_ROOM <name>     - Create chat room" << std::endl;
        std::cout << "  JOIN_ROOM <room_id>    - Join room" << std::endl;
        std::cout << "  SEND <room_id> <msg>   - Send message to room" << std::endl;
        std::cout << "  LEAVE_ROOM <room_id>   - Leave room" << std::endl;
        std::cout << std::endl;

        // 模拟运行（实际应该启动WebSocket服务器）
        run_demo();
    }

private:
    void run_demo() {
        std::cout << "=== IM Server Demo ===" << std::endl;
        std::cout << std::endl;

        // 1. 用户注册/生成Token
        std::cout << "[1] User Registration" << std::endl;
        auto alice_token = AuthManager::instance().generate_token("alice");
        auto bob_token = AuthManager::instance().generate_token("bob");
        std::cout << "  Alice token: " << alice_token.substr(0, 40) << "..." << std::endl;
        std::cout << "  Bob token: " << bob_token.substr(0, 40) << "..." << std::endl;
        std::cout << std::endl;

        // 2. 用户连接（认证）
        std::cout << "[2] User Connection" << std::endl;
        auto alice_id = authenticate(alice_token, "192.168.1.100");
        auto bob_id = authenticate(bob_token, "192.168.1.101");

        if (alice_id && bob_id) {
            std::cout << "  Alice connected (verified: " << *alice_id << ")" << std::endl;
            std::cout << "  Bob connected (verified: " << *bob_id << ")" << std::endl;
        }
        std::cout << std::endl;

        // 3. 创建房间
        std::cout << "[3] Create Room" << std::endl;
        auto room_id = RoomManager::instance().create_room("Dev Team", "alice");
        std::cout << "  Room created: " << room_id << std::endl;
        std::cout << "  Room name: Dev Team" << std::endl;
        std::cout << "  Owner: alice" << std::endl;
        std::cout << std::endl;

        // 4. 加入房间
        std::cout << "[4] Join Room" << std::endl;
        RoomManager::instance().join_room(room_id, "bob");
        auto room = RoomManager::instance().get_room(room_id);
        std::cout << "  Bob joined room" << std::endl;
        std::cout << "  Total members: " << room->member_count() << std::endl;
        std::cout << std::endl;

        // 5. 发送消息
        std::cout << "[5] Send Messages" << std::endl;

        // Alice发送消息
        ChatMessage msg1;
        msg1.content = "Hello everyone!";
        send_message("alice", room_id, msg1);

        // Bob回复
        ChatMessage msg2;
        msg2.content = "Hi Alice! Good to see you.";
        send_message("bob", room_id, msg2);

        std::cout << std::endl;

        // 6. 查看在线用户
        std::cout << "[6] Room Members" << std::endl;
        auto members = room->get_members();
        for (const auto& member : members) {
            std::cout << "  - " << member.user_id
                     << " (role: " << role_to_string(member.role) << ")" << std::endl;
        }
        std::cout << std::endl;

        // 7. 离线消息
        std::cout << "[7] Offline Messages" << std::endl;

        // 模拟charlie离线
        std::cout << "  Charlie is offline..." << std::endl;

        // Alice发消息给charlie（会存储为离线消息）
        StoredMessage offline_msg;
        offline_msg.message_id = Message::generate_message_id();
        offline_msg.from_user = "alice";
        offline_msg.to_user = "charlie";
        offline_msg.content = "Hey Charlie, check this out!";
        offline_msg.timestamp = Message::current_timestamp();
        offline_msg.delivered = false;

        MessageStorage::instance().store_message(offline_msg);
        std::cout << "  Message stored for charlie" << std::endl;

        // Charlie上线
        std::cout << "  Charlie comes online..." << std::endl;
        auto offline_msgs = MessageStorage::instance().get_offline_messages("charlie");
        std::cout << "  Charlie has " << offline_msgs.size() << " offline message(s)" << std::endl;

        for (const auto& msg : offline_msgs) {
            std::cout << "    From " << msg.from_user << ": " << msg.content << std::endl;
            MessageStorage::instance().mark_delivered(msg.message_id);
        }
        std::cout << std::endl;

        // 8. 统计信息
        std::cout << "[8] Server Statistics" << std::endl;
        std::cout << "  Total rooms: " << count_rooms() << std::endl;
        std::cout << "  Total users: " << count_online_users() << std::endl;
        std::cout << "  Messages sent: " << message_count_ << std::endl;
        std::cout << std::endl;

        std::cout << "=== Demo completed ===" << std::endl;
    }

    std::optional<std::string> authenticate(const std::string& token, const std::string& ip) {
        // 检查IP速率限制
        auto ip_result = IPRateLimiter::instance().check(ip);
        if (!ip_result.allowed) {
            std::cout << "  IP rate limit exceeded: " << ip << std::endl;
            return std::nullopt;
        }

        // 验证Token
        auto user_id = AuthManager::instance().verify_token(token);
        if (!user_id) {
            std::cout << "  Authentication failed" << std::endl;
            return std::nullopt;
        }

        // 检查用户速率限制
        auto user_result = UserRateLimiter::instance().check(*user_id);
        if (!user_result.allowed) {
            std::cout << "  User rate limit exceeded: " << *user_id << std::endl;
            return std::nullopt;
        }

        online_users_.insert(*user_id);
        return user_id;
    }

    void send_message(const std::string& sender, const std::string& room_id, const ChatMessage& msg) {
        auto room = RoomManager::instance().get_room(room_id);
        if (!room) {
            std::cout << "  Room not found: " << room_id << std::endl;
            return;
        }

        // 检查发送权限
        if (!room->can_send(sender)) {
            std::cout << "  " << sender << " cannot send (muted or no permission)" << std::endl;
            return;
        }

        // 获取广播目标
        auto targets = RoomManager::instance().get_broadcast_targets(room_id, sender);

        std::cout << "  " << sender << ": " << msg.content << std::endl;
        std::cout << "    -> Broadcast to " << targets.size() << " member(s)" << std::endl;

        message_count_++;
    }

    std::string role_to_string(MemberRole role) {
        switch (role) {
            case MemberRole::OWNER: return "Owner";
            case MemberRole::ADMIN: return "Admin";
            case MemberRole::MEMBER: return "Member";
            case MemberRole::GUEST: return "Guest";
            default: return "Unknown";
        }
    }

    int count_rooms() {
        return RoomManager::instance().list_rooms().size();
    }

    int count_online_users() {
        return online_users_.size();
    }

    uint16_t port_;
    std::set<std::string> online_users_;
    int message_count_ = 0;
};

int main(int argc, char* argv[]) {
    uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8080;

    try {
        IMServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
