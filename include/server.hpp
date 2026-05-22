#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include "io_context_pool.hpp"
#include <memory>
#include <string>
#include <atomic>

namespace gateway {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Connection;

class Server {
public:
    Server(asio::io_context& io_context, uint16_t port);
    Server(asio::io_context& acceptor_io_context,
           std::shared_ptr<IoContextPool> pool,
           uint16_t port);

    asio::awaitable<void> listen();

private:
    asio::awaitable<void> handle_client(tcp::socket socket);

    asio::io_context& acceptor_io_context_;
    std::shared_ptr<IoContextPool> pool_;
    tcp::acceptor acceptor_;
    std::atomic<uint64_t> next_conn_id_{1};
    bool use_pool_{false};
};

}
