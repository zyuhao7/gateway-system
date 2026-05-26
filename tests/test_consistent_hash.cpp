/**
 * @file test_consistent_hash.cpp
 * @brief 一致性哈希单元测试
 */

#include "consistent_hash.hpp"
#include <gtest/gtest.h>
#include <unordered_map>
#include <cmath>

class ConsistentHashTest : public ::testing::Test {
protected:
    void SetUp() override {
        hash_ = std::make_unique<ConsistentHash>(150);
    }

    std::unique_ptr<ConsistentHash> hash_;
};

// 测试节点添加
TEST_F(ConsistentHashTest, AddNode) {
    hash_->add_node("node1");
    hash_->add_node("node2");
    hash_->add_node("node3");

    // 验证可以获取节点
    std::string node = hash_->get_node("user_123");
    EXPECT_FALSE(node.empty());
}

// 测试节点移除
TEST_F(ConsistentHashTest, RemoveNode) {
    hash_->add_node("node1");
    hash_->add_node("node2");

    std::string node_before = hash_->get_node("user_123");

    hash_->remove_node("node1");

    std::string node_after = hash_->get_node("user_123");

    // 节点移除后，key 可能被重新分配
    EXPECT_FALSE(node_after.empty());
}

// 测试分布均匀性
TEST_F(ConsistentHashTest, Distribution) {
    hash_->add_node("node1");
    hash_->add_node("node2");
    hash_->add_node("node3");

    std::unordered_map<std::string, int> distribution;

    // 测试 1000 个 key 的分布
    for (int i = 0; i < 1000; i++) {
        std::string key = "user_" + std::to_string(i);
        std::string node = hash_->get_node(key);
        distribution[node]++;
    }

    // 验证每个节点都有分配
    EXPECT_EQ(distribution.size(), 3);

    // 验证分布相对均匀（每个节点应该在 200-500 之间）
    for (const auto& [node, count] : distribution) {
        EXPECT_GT(count, 200);
        EXPECT_LT(count, 500);
        std::cout << node << ": " << count << " keys" << std::endl;
    }
}

// 测试一致性（节点变化时只影响部分 key）
TEST_F(ConsistentHashTest, Consistency) {
    hash_->add_node("node1");
    hash_->add_node("node2");
    hash_->add_node("node3");

    // 记录 1000 个 key 的初始分配
    std::unordered_map<std::string, std::string> initial_mapping;
    for (int i = 0; i < 1000; i++) {
        std::string key = "user_" + std::to_string(i);
        initial_mapping[key] = hash_->get_node(key);
    }

    // 移除一个节点
    hash_->remove_node("node2");

    // 检查有多少 key 被重新分配
    int changed = 0;
    for (int i = 0; i < 1000; i++) {
        std::string key = "user_" + std::to_string(i);
        std::string new_node = hash_->get_node(key);
        if (initial_mapping[key] != new_node) {
            changed++;
        }
    }

    // 理论上应该只有约 1/3 的 key 被重新分配
    // 实际测试中允许 20%-50% 的范围
    double change_ratio = static_cast<double>(changed) / 1000.0;
    std::cout << "Changed keys: " << changed << " (" << (change_ratio * 100) << "%)" << std::endl;

    EXPECT_GT(change_ratio, 0.2);
    EXPECT_LT(change_ratio, 0.5);
}

// 测试空节点情况
TEST_F(ConsistentHashTest, EmptyNodes) {
    std::string node = hash_->get_node("user_123");
    EXPECT_TRUE(node.empty());
}

// 测试单节点
TEST_F(ConsistentHashTest, SingleNode) {
    hash_->add_node("node1");

    // 所有 key 都应该分配到同一个节点
    std::string node1 = hash_->get_node("user_1");
    std::string node2 = hash_->get_node("user_2");
    std::string node3 = hash_->get_node("user_3");

    EXPECT_EQ(node1, "node1");
    EXPECT_EQ(node2, "node1");
    EXPECT_EQ(node3, "node1");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
