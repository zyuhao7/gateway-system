/**
 * @file websocket_connection.hpp
 * @brief WebSocket 连接管理
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include <string>
#include <chrono>

namespace gateway {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class WebSocketConnection : public std::enable_shared_from_this<WebSocketConnection> {
public:
    WebSocketConnection(tcp::socket socket, uint64_t id);

    asio::awaitable<void> start();

    uint64_t id() const { return id_; }

    asio::awaitable<void> send_message(const std::string& msg);
    asio::awaitable<void> send_binary(const std::vector<uint8_t>& data);

    void close();

private:
    asio::awaitable<void> read_loop();
    asio::awaitable<void> heartbeat_loop();
    asio::awaitable<void> handle_message(const std::string& msg);

    websocket::stream<tcp::socket> ws_;
    uint64_t id_;
    asio::steady_timer heartbeat_timer_;
    std::chrono::steady_clock::time_point last_activity_;

    static constexpr std::chrono::seconds heartbeat_interval_{30};
    static constexpr std::chrono::seconds timeout_{90};
};

}
