#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <string>
#include <chrono>

namespace gateway {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket socket, uint64_t id);

    asio::awaitable<void> start();

    uint64_t id() const { return id_; }

private:
    asio::awaitable<void> read_loop();
    asio::awaitable<void> heartbeat_loop();
    asio::awaitable<void> write_message(const std::string& msg);

    tcp::socket socket_;
    uint64_t id_;
    asio::steady_timer heartbeat_timer_;
    std::chrono::steady_clock::time_point last_activity_;

    static constexpr std::chrono::seconds heartbeat_interval_{30};
    static constexpr std::chrono::seconds timeout_{90};
};

}
