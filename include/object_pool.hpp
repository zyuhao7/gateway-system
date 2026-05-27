/**
 * @file object_pool.hpp
 * @brief 通用对象池实现
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>

template<typename T>
class ObjectPool {
public:
    using Deleter = std::function<void(T*)>;
    using UniquePtr = std::unique_ptr<T, Deleter>;

    explicit ObjectPool(size_t initial_size = 100, size_t max_size = 10000)
        : max_size_(max_size)
        , allocated_count_(0)
        , acquired_count_(0)
    {
        pool_.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i) {
            pool_.push_back(std::make_unique<T>());
        }
        allocated_count_ = initial_size;
    }

    ~ObjectPool() = default;

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    UniquePtr acquire() {
        std::unique_lock<std::mutex> lock(mutex_);

        if (pool_.empty()) {
            if (allocated_count_ < max_size_) {
                ++allocated_count_;
                ++acquired_count_;
                lock.unlock();

                return UniquePtr(new T(), [this](T* ptr) {
                    this->release(ptr);
                });
            } else {
                return nullptr;
            }
        }

        auto obj = std::move(pool_.back());
        pool_.pop_back();
        ++acquired_count_;

        T* raw_ptr = obj.release();
        return UniquePtr(raw_ptr, [this](T* ptr) {
            this->release(ptr);
        });
    }

    template<typename... Args>
    UniquePtr acquire_with_init(Args&&... args) {
        auto obj = acquire();
        if (obj) {
            obj->reset(std::forward<Args>(args)...);
        }
        return obj;
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    size_t allocated() const {
        return allocated_count_.load();
    }

    size_t acquired() const {
        return acquired_count_.load();
    }

    size_t in_use() const {
        return acquired_count_.load() - available();
    }

private:
    void release(T* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex_);
        --acquired_count_;

        if (pool_.size() < max_size_) {
            pool_.push_back(std::unique_ptr<T>(ptr));
        } else {
            delete ptr;
            --allocated_count_;
        }
    }

    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<T>> pool_;
    size_t max_size_;
    std::atomic<size_t> allocated_count_;
    std::atomic<size_t> acquired_count_;
};
