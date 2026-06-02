/**
 * @file push_server.cpp
 * @brief 推送服务器示例 - 订阅管理和消息推送
 */

#include "rate_limiter.hpp"
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <random>

using namespace gateway;

// 订阅信息
struct Subscription {
    std::string user_id;
    std::set<std::string> topics;
    std::set<std::string> tags;
};

class PushServer {
public:
    PushServer(uint16_t port) : port_(port) {
        // 配置速率限制
        UserRateLimiter::instance().configure(1000, 1500);
        std::cout << "Push Server initialized on port " << port_ << std::endl;
    }

    void start() {
        std::cout << "\n=== Push Server Demo ===" << std::endl;
        std::cout << "\nSupported Features:" << std::endl;
        std::cout << "  - Topic Subscription" << std::endl;
        std::cout << "  - Tag-based Targeting" << std::endl;
        std::cout << "  - Scheduled Push" << std::endl;
        std::cout << "  - Delivery Tracking" << std::endl;
        std::cout << std::endl;

        run_demo();
    }

private:
    void run_demo() {
        // 1. 用户订阅
        std::cout << "[1] User Subscriptions" << std::endl;

        subscribe("user_001", {"news", "sports"}, {"vip", "android"});
        subscribe("user_002", {"news", "tech"}, {"premium", "ios"});
        subscribe("user_003", {"sports", "entertainment"}, {"free", "android"});

        std::cout << "  Total subscribers: " << subscriptions_.size() << std::endl;
        std::cout << std::endl;

        // 2. 主题推送
        std::cout << "[2] Topic Push" << std::endl;

        push_to_topic("news", "Breaking News", "Major event happening now!");
        std::cout << std::endl;

        // 3. 标签推送
        std::cout << "[3] Tag-based Push" << std::endl;

        push_to_tags({"vip"}, "VIP Exclusive", "Special offer just for you!");
        std::cout << std::endl;

        // 4. 单播推送
        std::cout << "[4] Unicast Push" << std::endl;

        push_to_user("user_002", "Personal Message", "You have 3 new notifications");
        std::cout << std::endl;

        // 5. 广播推送
        std::cout << "[5] Broadcast Push" << std::endl;

        broadcast("System Announcement", "Server maintenance at 2 AM");
        std::cout << std::endl;

        // 6. 定时推送
        std::cout << "[6] Scheduled Push" << std::endl;

        schedule_push("news", "Morning Briefing", "Your daily news summary", "08:00");
        std::cout << std::endl;

        // 7. 推送统计
        std::cout << "[7] Push Statistics" << std::endl;

        show_statistics();
        std::cout << std::endl;

        // 8. 取消订阅
        std::cout << "[8] Unsubscribe" << std::endl;

        unsubscribe("user_003", "sports");
        std::cout << "  user_003 unsubscribed from 'sports'" << std::endl;

        auto& sub = subscriptions_["user_003"];
        std::cout << "  Remaining topics: ";
        for (const auto& topic : sub.topics) {
            std::cout << topic << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;

        std::cout << "=== Demo completed ===" << std::endl;
    }

    void subscribe(const std::string& user_id,
                   const std::set<std::string>& topics,
                   const std::set<std::string>& tags) {
        Subscription sub;
        sub.user_id = user_id;
        sub.topics = topics;
        sub.tags = tags;

        subscriptions_[user_id] = sub;

        std::cout << "  " << user_id << " subscribed to:" << std::endl;
        std::cout << "    Topics: ";
        for (const auto& topic : topics) {
            std::cout << topic << " ";
            topic_subscribers_[topic].insert(user_id);
        }
        std::cout << std::endl;

        std::cout << "    Tags: ";
        for (const auto& tag : tags) {
            std::cout << tag << " ";
            tag_users_[tag].insert(user_id);
        }
        std::cout << std::endl;
    }

    void unsubscribe(const std::string& user_id, const std::string& topic) {
        auto it = subscriptions_.find(user_id);
        if (it != subscriptions_.end()) {
            it->second.topics.erase(topic);
            topic_subscribers_[topic].erase(user_id);
        }
    }

    void push_to_topic(const std::string& topic,
                      const std::string& title,
                      const std::string& content) {
        auto it = topic_subscribers_.find(topic);
        if (it == topic_subscribers_.end()) {
            std::cout << "  No subscribers for topic: " << topic << std::endl;
            return;
        }

        std::cout << "  Pushing to topic '" << topic << "'" << std::endl;
        std::cout << "  Title: " << title << std::endl;
        std::cout << "  Content: " << content << std::endl;

        int delivered = 0;
        for (const auto& user_id : it->second) {
            if (deliver_push(user_id, title, content)) {
                delivered++;
            }
        }

        std::cout << "  Delivered: " << delivered << "/" << it->second.size() << std::endl;
        total_pushes_ += delivered;
    }

    void push_to_tags(const std::set<std::string>& tags,
                     const std::string& title,
                     const std::string& content) {
        std::set<std::string> target_users;

        std::cout << "  Pushing to tags: ";
        for (const auto& tag : tags) {
            std::cout << tag << " ";
            auto it = tag_users_.find(tag);
            if (it != tag_users_.end()) {
                target_users.insert(it->second.begin(), it->second.end());
            }
        }
        std::cout << std::endl;

        std::cout << "  Title: " << title << std::endl;
        std::cout << "  Content: " << content << std::endl;

        int delivered = 0;
        for (const auto& user_id : target_users) {
            if (deliver_push(user_id, title, content)) {
                delivered++;
            }
        }

        std::cout << "  Delivered: " << delivered << "/" << target_users.size() << std::endl;
        total_pushes_ += delivered;
    }

    void push_to_user(const std::string& user_id,
                     const std::string& title,
                     const std::string& content) {
        std::cout << "  Pushing to user: " << user_id << std::endl;
        std::cout << "  Title: " << title << std::endl;
        std::cout << "  Content: " << content << std::endl;

        if (deliver_push(user_id, title, content)) {
            std::cout << "  Status: Delivered" << std::endl;
            total_pushes_++;
        } else {
            std::cout << "  Status: Failed" << std::endl;
        }
    }

    void broadcast(const std::string& title, const std::string& content) {
        std::cout << "  Broadcasting to all users" << std::endl;
        std::cout << "  Title: " << title << std::endl;
        std::cout << "  Content: " << content << std::endl;

        int delivered = 0;
        for (const auto& [user_id, _] : subscriptions_) {
            if (deliver_push(user_id, title, content)) {
                delivered++;
            }
        }

        std::cout << "  Delivered: " << delivered << "/" << subscriptions_.size() << std::endl;
        total_pushes_ += delivered;
    }

    void schedule_push(const std::string& topic,
                      const std::string& title,
                      const std::string& content,
                      const std::string& schedule_time) {
        std::cout << "  Scheduled push created:" << std::endl;
        std::cout << "    Topic: " << topic << std::endl;
        std::cout << "    Title: " << title << std::endl;
        std::cout << "    Schedule: " << schedule_time << std::endl;

        scheduled_pushes_++;
    }

    bool deliver_push(const std::string& user_id,
                     const std::string& title,
                     const std::string& content) {
        // 检查速率限制
        auto result = UserRateLimiter::instance().check(user_id);
        if (!result.allowed) {
            return false;
        }

        // 模拟推送成功率（90%）
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        return dis(gen) < 0.9;
    }

    void show_statistics() {
        std::cout << "  Total subscribers: " << subscriptions_.size() << std::endl;
        std::cout << "  Total pushes sent: " << total_pushes_ << std::endl;
        std::cout << "  Scheduled pushes: " << scheduled_pushes_ << std::endl;

        double delivery_rate = subscriptions_.empty() ? 0.0 :
            (double)total_pushes_ / (subscriptions_.size() * 4) * 100.0;
        std::cout << "  Delivery rate: " << delivery_rate << "%" << std::endl;
    }

    uint16_t port_;
    std::map<std::string, Subscription> subscriptions_;
    std::map<std::string, std::set<std::string>> topic_subscribers_;
    std::map<std::string, std::set<std::string>> tag_users_;
    int total_pushes_ = 0;
    int scheduled_pushes_ = 0;
};

int main(int argc, char* argv[]) {
    uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8083;

    try {
        PushServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
