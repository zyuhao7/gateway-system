/**
 * @file websocket_server.hpp
 * @brief WebSocket 服务器
 */

#pragma once

#include "io_context_pool.hpp"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <memory>
#include <atomic>

namespace gateway {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class WebSocketServer {
public:
    WebSocketServer(asio::io_context& io_context, uint16_t port);
    WebSocketServer(asio::io_context& io_context,
                    std::shared_ptr<IoContextPool> pool,
                    uint16_t port);

    asio::awaitable<void> listen();

private:
    asio::awaitable<void> accept_connection();

    asio::io_context& io_context_;
    std::shared_ptr<IoContextPool> pool_;
    tcp::acceptor acceptor_;
    std::atomic<uint64_t> next_connection_id_{1};
    bool use_pool_;
};

}
