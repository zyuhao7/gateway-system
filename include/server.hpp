#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <memory>
#include <string>

namespace gateway {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Connection;

class Server {
public:
    Server(asio::io_context& io_context, uint16_t port);

    asio::awaitable<void> listen();

private:
    asio::awaitable<void> handle_client(tcp::socket socket);

    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    uint64_t next_conn_id_{1};
};

}
