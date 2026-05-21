#include "connection.hpp"
#include <iostream>
#include <vector>

namespace gateway {

Connection::Connection(tcp::socket socket, uint64_t id)
    : socket_(std::move(socket))
    , id_(id)
    , heartbeat_timer_(socket_.get_executor())
    , last_activity_(std::chrono::steady_clock::now()) {
}

asio::awaitable<void> Connection::start() {
    co_await write_message("WELCOME\n");

    asio::co_spawn(socket_.get_executor(),
                  heartbeat_loop(),
                  asio::detached);

    co_await read_loop();
}

asio::awaitable<void> Connection::read_loop() {
    std::vector<char> buffer(4096);

    while (true) {
        size_t n = co_await socket_.async_read_some(
            asio::buffer(buffer),
            asio::use_awaitable
        );

        last_activity_ = std::chrono::steady_clock::now();

        std::string msg(buffer.data(), n);
        std::cout << "Connection " << id_ << " received: " << msg;

        if (msg.find("PING") != std::string::npos) {
            co_await write_message("PONG\n");
        } else if (msg.find("QUIT") != std::string::npos) {
            co_await write_message("BYE\n");
            break;
        } else {
            co_await write_message("ECHO: " + msg);
        }
    }
}

asio::awaitable<void> Connection::heartbeat_loop() {
    while (true) {
        heartbeat_timer_.expires_after(heartbeat_interval_);
        co_await heartbeat_timer_.async_wait(asio::use_awaitable);

        auto now = std::chrono::steady_clock::now();
        auto idle = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_);

        if (idle > timeout_) {
            std::cout << "Connection " << id_ << " timeout, closing" << std::endl;
            socket_.close();
            break;
        }
    }
}

asio::awaitable<void> Connection::write_message(const std::string& msg) {
    co_await asio::async_write(
        socket_,
        asio::buffer(msg),
        asio::use_awaitable
    );
}

}
