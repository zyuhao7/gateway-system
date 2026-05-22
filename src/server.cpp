#include "server.hpp"
#include "connection.hpp"
#include "connection_manager.hpp"
#include "io_context_pool.hpp"
#include <iostream>

namespace gateway {

Server::Server(asio::io_context& io_context, uint16_t port)
    : acceptor_io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    , use_pool_(false) {
    std::cout << "Server listening on port " << port << " (single-threaded)" << std::endl;
}

Server::Server(asio::io_context& acceptor_io_context,
               std::shared_ptr<IoContextPool> pool,
               uint16_t port)
    : acceptor_io_context_(acceptor_io_context)
    , pool_(pool)
    , acceptor_(acceptor_io_context, tcp::endpoint(tcp::v4(), port))
    , use_pool_(true) {
    std::cout << "Server listening on port " << port
              << " (multi-threaded, pool size: " << pool->size() << ")" << std::endl;
}

asio::awaitable<void> Server::listen() {
    while (true) {
        tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

        auto endpoint = socket.remote_endpoint();
        uint64_t conn_id = next_conn_id_++;
        std::cout << "New connection from " << endpoint.address().to_string()
                  << ":" << endpoint.port() << " (id=" << conn_id << ")" << std::endl;

        if (use_pool_) {
            auto& io_context = pool_->get_io_context_for_connection(conn_id);
            asio::co_spawn(io_context,
                          handle_client(std::move(socket)),
                          asio::detached);
        } else {
            asio::co_spawn(acceptor_io_context_,
                          handle_client(std::move(socket)),
                          asio::detached);
        }
    }
}

asio::awaitable<void> Server::handle_client(tcp::socket socket) {
    uint64_t conn_id = next_conn_id_++;
    auto conn = std::make_shared<Connection>(std::move(socket), conn_id);

    ConnectionManager::instance().add(conn_id, conn);

    try {
        co_await conn->start();
    } catch (const std::exception& e) {
        std::cerr << "Connection " << conn_id << " error: " << e.what() << std::endl;
    }

    ConnectionManager::instance().remove(conn_id);
    std::cout << "Connection " << conn_id << " closed" << std::endl;
}

}
