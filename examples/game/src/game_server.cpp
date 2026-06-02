/**
 * @file game_server.cpp
 * @brief 游戏服务器示例 - 房间匹配和状态同步
 */

#include "room.hpp"
#include "rate_limiter.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>

using namespace gateway;

// 游戏房间扩展
class GameRoom {
public:
    GameRoom(const std::string& room_id) : room_id_(room_id), state_("waiting") {}

    void start_game() {
        state_ = "playing";
        start_time_ = std::chrono::steady_clock::now();
        std::cout << "  Game started in room " << room_id_ << std::endl;
    }

    void update_player_state(const std::string& player_id, int score) {
        player_scores_[player_id] = score;
    }

    void end_game() {
        state_ = "finished";
        std::cout << "  Game ended in room " << room_id_ << std::endl;
        show_results();
    }

    void show_results() {
        std::cout << "  Final Scores:" << std::endl;
        for (const auto& [player, score] : player_scores_) {
            std::cout << "    " << player << ": " << score << " points" << std::endl;
        }
    }

private:
    std::string room_id_;
    std::string state_;
    std::chrono::steady_clock::time_point start_time_;
    std::map<std::string, int> player_scores_;
};

class GameServer {
public:
    GameServer(uint16_t port) : port_(port) {
        // 配置速率限制
        UserRateLimiter::instance().configure(60, 100);  // 60 ops/min
        std::cout << "Game Server initialized on port " << port_ << std::endl;
    }

    void start() {
        std::cout << "\n=== Game Server Demo ===" << std::endl;
        std::cout << "\nGame Modes:" << std::endl;
        std::cout << "  - Battle Royale (max 4 players)" << std::endl;
        std::cout << "  - Team Deathmatch (2v2)" << std::endl;
        std::cout << "  - Free For All" << std::endl;
        std::cout << std::endl;

        run_demo();
    }

private:
    void run_demo() {
        // 1. 创建匹配队列
        std::cout << "[1] Players Entering Matchmaking" << std::endl;
        std::vector<std::string> players = {"player1", "player2", "player3", "player4"};

        for (const auto& player : players) {
            std::cout << "  " << player << " joined matchmaking" << std::endl;
        }
        std::cout << "  Queue size: " << players.size() << std::endl;
        std::cout << std::endl;

        // 2. 匹配成功，创建房间
        std::cout << "[2] Match Found - Creating Room" << std::endl;
        auto room_id = RoomManager::instance().create_room(
            "Battle Royale #1",
            players[0],
            RoomType::PRIVATE
        );

        auto room = RoomManager::instance().get_room(room_id);
        room->set_max_members(4);

        std::cout << "  Room ID: " << room_id << std::endl;
        std::cout << "  Mode: Battle Royale" << std::endl;
        std::cout << "  Max Players: 4" << std::endl;

        // 其他玩家加入
        for (size_t i = 1; i < players.size(); ++i) {
            RoomManager::instance().join_room(room_id, players[i]);
        }
        std::cout << "  All players joined (" << room->member_count() << "/4)" << std::endl;
        std::cout << std::endl;

        // 3. 游戏开始
        std::cout << "[3] Game Starting" << std::endl;
        GameRoom game(room_id);
        game.start_game();

        std::cout << "  Map: Desert Arena" << std::endl;
        std::cout << "  Duration: 5 minutes" << std::endl;
        std::cout << std::endl;

        // 4. 游戏进行中 - 状态同步
        std::cout << "[4] Game In Progress - State Sync" << std::endl;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> score_dist(100, 500);

        for (int round = 1; round <= 3; ++round) {
            std::cout << "  Round " << round << ":" << std::endl;

            for (const auto& player : players) {
                int score = score_dist(gen);
                game.update_player_state(player, score);
                std::cout << "    " << player << " scored " << score << " points" << std::endl;

                // 广播状态给其他玩家
                auto targets = RoomManager::instance().get_broadcast_targets(room_id, player);
                // 模拟状态同步
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << std::endl;

        // 5. 游戏结束
        std::cout << "[5] Game Ending" << std::endl;
        game.end_game();
        std::cout << std::endl;

        // 6. 玩家离开
        std::cout << "[6] Players Leaving" << std::endl;
        for (const auto& player : players) {
            RoomManager::instance().leave_room(room_id, player);
            std::cout << "  " << player << " left the room" << std::endl;
        }
        std::cout << std::endl;

        // 7. 清理房间
        std::cout << "[7] Cleanup" << std::endl;
        RoomManager::instance().delete_room(room_id);
        std::cout << "  Room deleted" << std::endl;
        std::cout << std::endl;

        std::cout << "=== Demo completed ===" << std::endl;
    }

    uint16_t port_;
};

int main(int argc, char* argv[]) {
    uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8081;

    try {
        GameServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
