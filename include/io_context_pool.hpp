#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>

namespace asio = boost::asio;

class IoContextPool {
public:
    explicit IoContextPool(size_t pool_size);
    ~IoContextPool();

    IoContextPool(const IoContextPool&) = delete;
    IoContextPool& operator=(const IoContextPool&) = delete;

    void run();
    void stop();

    asio::io_context& get_io_context();
    asio::io_context& get_io_context_for_connection(uint64_t conn_id);

    size_t size() const { return pool_size_; }

private:
    size_t pool_size_;
    std::vector<std::shared_ptr<asio::io_context>> io_contexts_;
    std::vector<std::shared_ptr<asio::io_context::work>> work_guards_;
    std::vector<std::thread> threads_;
    std::atomic<size_t> next_io_context_{0};
};
