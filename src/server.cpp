#include "server.hpp"
#include "connection.hpp"
#include "connection_manager.hpp"
#include <iostream>

namespace gateway {

Server::Server(asio::io_context& io_context, uint16_t port)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "Server listening on port " << port << std::endl;
}

asio::awaitable<void> Server::listen() {
    while (true) {
        tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

        auto endpoint = socket.remote_endpoint();
        std::cout << "New connection from " << endpoint.address().to_string()
                  << ":" << endpoint.port() << " (id=" << next_conn_id_ << ")" << std::endl;

        asio::co_spawn(io_context_,
                      handle_client(std::move(socket)),
                      asio::detached);
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
