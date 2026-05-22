#include "io_context_pool.hpp"
#include <stdexcept>

IoContextPool::IoContextPool(size_t pool_size)
    : pool_size_(pool_size) {
    if (pool_size == 0) {
        throw std::runtime_error("IoContextPool size must be > 0");
    }

    for (size_t i = 0; i < pool_size_; ++i) {
        auto io_context = std::make_shared<asio::io_context>();
        io_contexts_.push_back(io_context);
        work_guards_.push_back(
            std::make_shared<asio::io_context::work>(*io_context)
        );
    }
}

IoContextPool::~IoContextPool() {
    stop();
}

void IoContextPool::run() {
    for (size_t i = 0; i < pool_size_; ++i) {
        threads_.emplace_back([this, i]() {
            io_contexts_[i]->run();
        });
    }
}

void IoContextPool::stop() {
    work_guards_.clear();

    for (auto& io_context : io_contexts_) {
        io_context->stop();
    }

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

asio::io_context& IoContextPool::get_io_context() {
    size_t index = next_io_context_.fetch_add(1, std::memory_order_relaxed) % pool_size_;
    return *io_contexts_[index];
}

asio::io_context& IoContextPool::get_io_context_for_connection(uint64_t conn_id) {
    size_t index = conn_id % pool_size_;
    return *io_contexts_[index];
}
