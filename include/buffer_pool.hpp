/**
 * @file buffer_pool.hpp
 * @brief 缓冲区对象池，用于减少频繁的内存分配
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <atomic>

class BufferPool {
public:
    static constexpr size_t DEFAULT_BUFFER_SIZE = 4096;
    static constexpr size_t DEFAULT_POOL_SIZE = 1000;
    static constexpr size_t MAX_POOL_SIZE = 10000;

    struct Buffer {
        std::vector<char> data;

        Buffer() : data(DEFAULT_BUFFER_SIZE) {}

        void reset() {
            data.clear();
            data.resize(DEFAULT_BUFFER_SIZE);
        }

        char* ptr() { return data.data(); }
        size_t size() const { return data.size(); }
        void resize(size_t new_size) { data.resize(new_size); }
    };

    using BufferPtr = std::shared_ptr<Buffer>;

    static BufferPool& instance() {
        static BufferPool pool;
        return pool;
    }

    BufferPtr acquire() {
        std::unique_lock<std::mutex> lock(mutex_);

        if (pool_.empty()) {
            ++allocated_count_;
            ++acquired_count_;
            lock.unlock();

            auto buffer = std::make_shared<Buffer>();
            return std::shared_ptr<Buffer>(buffer.get(), [this, buffer](Buffer*) mutable {
                this->release(std::move(buffer));
            });
        }

        auto buffer = std::move(pool_.back());
        pool_.pop_back();
        ++acquired_count_;

        return std::shared_ptr<Buffer>(buffer.get(), [this, buffer](Buffer*) mutable {
            this->release(std::move(buffer));
        });
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    size_t allocated() const {
        return allocated_count_.load();
    }

    size_t in_use() const {
        return acquired_count_.load();
    }

private:
    BufferPool() : allocated_count_(0), acquired_count_(0) {
        pool_.reserve(DEFAULT_POOL_SIZE);
        for (size_t i = 0; i < DEFAULT_POOL_SIZE; ++i) {
            pool_.push_back(std::make_shared<Buffer>());
        }
        allocated_count_ = DEFAULT_POOL_SIZE;
    }

    void release(std::shared_ptr<Buffer> buffer) {
        if (!buffer) return;

        buffer->reset();

        std::lock_guard<std::mutex> lock(mutex_);
        --acquired_count_;

        if (pool_.size() < MAX_POOL_SIZE) {
            pool_.push_back(std::move(buffer));
        } else {
            --allocated_count_;
        }
    }

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Buffer>> pool_;
    std::atomic<size_t> allocated_count_;
    std::atomic<size_t> acquired_count_;
};
