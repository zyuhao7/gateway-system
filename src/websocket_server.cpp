/**
 * @file websocket_server.cpp
 * @brief WebSocket 服务器实现
 */

#include "websocket_server.hpp"
#include "websocket_connection.hpp"
#include "connection_manager.hpp"
#include "io_context_pool.hpp"
#include "metrics.hpp"
#include <iostream>

namespace gateway {

WebSocketServer::WebSocketServer(asio::io_context& io_context, uint16_t port)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    , use_pool_(false) {
    std::cout << "WebSocket server listening on port " << port << " (single-threaded)" << std::endl;
}

WebSocketServer::WebSocketServer(asio::io_context& io_context,
                                 std::shared_ptr<IoContextPool> pool,
                                 uint16_t port)
    : io_context_(io_context)
    , pool_(pool)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    , use_pool_(true) {
    std::cout << "WebSocket server listening on port " << port << " (multi-threaded)" << std::endl;
}

asio::awaitable<void> WebSocketServer::listen() {
    while (true) {
        co_await accept_connection();
    }
}

asio::awaitable<void> WebSocketServer::accept_connection() {
    tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

    uint64_t conn_id = next_connection_id_++;

    std::cout << "WebSocket connection accepted: " << conn_id << std::endl;

    Metrics::instance().connection_opened();

    if (use_pool_ && pool_) {
        // 多线程模式：将连接分配到工作线程
        auto& worker_io_context = pool_->get_io_context_for_connection(conn_id);

        asio::co_spawn(worker_io_context,
            [socket = std::move(socket), conn_id]() mutable -> asio::awaitable<void> {
                auto conn = std::make_shared<WebSocketConnection>(std::move(socket), conn_id);

                co_await conn->start();

                Metrics::instance().connection_closed();
            },
            asio::detached);

    } else {
        // 单线程模式
        asio::co_spawn(io_context_,
            [socket = std::move(socket), conn_id]() mutable -> asio::awaitable<void> {
                auto conn = std::make_shared<WebSocketConnection>(std::move(socket), conn_id);

                co_await conn->start();

                Metrics::instance().connection_closed();
            },
            asio::detached);
    }
}

}
