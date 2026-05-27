/**
 * @file test_buffer_pool.cpp
 * @brief 缓冲池单元测试
 */

#include "buffer_pool.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class BufferPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // BufferPool 是单例，无需特殊设置
    }
};

// 测试基本的获取和释放
TEST_F(BufferPoolTest, AcquireAndRelease) {
    auto& pool = BufferPool::instance();

    size_t initial_available = pool.available();

    {
        auto buffer = pool.acquire();
        ASSERT_NE(buffer, nullptr);
        EXPECT_EQ(buffer->size(), BufferPool::DEFAULT_BUFFER_SIZE);
        EXPECT_EQ(pool.available(), initial_available - 1);
    }

    // 缓冲区应该被归还到池中
    EXPECT_EQ(pool.available(), initial_available);
}

// 测试多个缓冲区
TEST_F(BufferPoolTest, MultipleBuffers) {
    auto& pool = BufferPool::instance();

    std::vector<BufferPool::BufferPtr> buffers;

    for (int i = 0; i < 10; i++) {
        auto buffer = pool.acquire();
        ASSERT_NE(buffer, nullptr);
        buffers.push_back(std::move(buffer));
    }

    EXPECT_EQ(buffers.size(), 10);

    // 释放所有缓冲区
    buffers.clear();

    // 验证缓冲区被归还
    EXPECT_GT(pool.available(), 0);
}

// 测试缓冲区重置
TEST_F(BufferPoolTest, BufferReset) {
    auto& pool = BufferPool::instance();

    auto buffer = pool.acquire();
    ASSERT_NE(buffer, nullptr);

    // 修改缓冲区
    buffer->data[0] = 'X';
    buffer->data[1] = 'Y';
    buffer->data[2] = 'Z';

    auto ptr = buffer->ptr();
    buffer.reset();

    // 重新获取缓冲区（可能是同一个）
    auto buffer2 = pool.acquire();
    ASSERT_NE(buffer2, nullptr);

    // 验证缓冲区大小正确
    EXPECT_EQ(buffer2->size(), BufferPool::DEFAULT_BUFFER_SIZE);
}

// 测试线程安全
TEST_F(BufferPoolTest, ThreadSafety) {
    auto& pool = BufferPool::instance();

    const int num_threads = 10;
    const int ops_per_thread = 100;

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&pool, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; j++) {
                auto buffer = pool.acquire();
                ASSERT_NE(buffer, nullptr);

                // 使用缓冲区
                buffer->data[0] = 'A';

                // 缓冲区会在作用域结束时自动释放
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 验证所有缓冲区都被归还
    EXPECT_GT(pool.available(), 0);
}

// 测试统计信息
TEST_F(BufferPoolTest, Statistics) {
    auto& pool = BufferPool::instance();

    size_t initial_allocated = pool.allocated();

    std::vector<BufferPool::BufferPtr> buffers;

    // 获取一些缓冲区
    for (int i = 0; i < 5; i++) {
        buffers.push_back(pool.acquire());
    }

    EXPECT_EQ(pool.in_use(), 5);
    EXPECT_GE(pool.allocated(), initial_allocated);

    // 释放缓冲区
    buffers.clear();

    EXPECT_EQ(pool.in_use(), 0);
}

// 测试缓冲区读写
TEST_F(BufferPoolTest, BufferReadWrite) {
    auto& pool = BufferPool::instance();

    auto buffer = pool.acquire();
    ASSERT_NE(buffer, nullptr);

    // 写入数据
    const char* test_data = "Hello, BufferPool!";
    size_t len = strlen(test_data);
    memcpy(buffer->ptr(), test_data, len);

    // 读取数据
    std::string result(buffer->ptr(), len);
    EXPECT_EQ(result, test_data);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
