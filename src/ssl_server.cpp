/**
 * @file ssl_server.cpp
 * @brief TLS/SSL 服务器实现
 */

#include "ssl_server.hpp"
#include "connection.hpp"
#include "connection_manager.hpp"
#include "io_context_pool.hpp"
#include "metrics.hpp"
#include <iostream>

namespace gateway {

SslServer::SslServer(asio::io_context& io_context,
                     uint16_t port,
                     const std::string& cert_file,
                     const std::string& key_file)
    : acceptor_io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    , ssl_context_(ssl::context::tlsv13)
    , use_pool_(false) {

    init_ssl_context(cert_file, key_file);
    std::cout << "SSL Server listening on port " << port << " (single-threaded)" << std::endl;
}

SslServer::SslServer(asio::io_context& acceptor_io_context,
                     std::shared_ptr<IoContextPool> pool,
                     uint16_t port,
                     const std::string& cert_file,
                     const std::string& key_file)
    : acceptor_io_context_(acceptor_io_context)
    , pool_(pool)
    , acceptor_(acceptor_io_context, tcp::endpoint(tcp::v4(), port))
    , ssl_context_(ssl::context::tlsv13)
    , use_pool_(true) {

    init_ssl_context(cert_file, key_file);
    std::cout << "SSL Server listening on port " << port
              << " (multi-threaded, pool size: " << pool->size() << ")" << std::endl;
}

void SslServer::init_ssl_context(const std::string& cert_file, const std::string& key_file) {
    ssl_context_.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::no_sslv3 |
        ssl::context::single_dh_use
    );

    ssl_context_.use_certificate_chain_file(cert_file);
    ssl_context_.use_private_key_file(key_file, ssl::context::pem);

    std::cout << "SSL context initialized with cert: " << cert_file << std::endl;
}

asio::awaitable<void> SslServer::listen() {
    while (true) {
        tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

        auto endpoint = socket.remote_endpoint();
        uint64_t conn_id = next_conn_id_++;
        std::cout << "New SSL connection from " << endpoint.address().to_string()
                  << ":" << endpoint.port() << " (id=" << conn_id << ")" << std::endl;

        // 创建 SSL stream
        ssl::stream<tcp::socket> ssl_socket(std::move(socket), ssl_context_);

        if (use_pool_) {
            auto& io_context = pool_->get_io_context_for_connection(conn_id);
            asio::co_spawn(io_context,
                          handle_client(std::move(ssl_socket)),
                          asio::detached);
        } else {
            asio::co_spawn(acceptor_io_context_,
                          handle_client(std::move(ssl_socket)),
                          asio::detached);
        }
    }
}

asio::awaitable<void> SslServer::handle_client(ssl::stream<tcp::socket> ssl_socket) {
    uint64_t conn_id = next_conn_id_++;

    try {
        // 执行 SSL 握手
        co_await ssl_socket.async_handshake(ssl::stream_base::server, asio::use_awaitable);
        std::cout << "SSL handshake completed for connection " << conn_id << std::endl;

        // 提取底层 socket 并创建 Connection
        // 注意：这里简化处理，实际应该创建支持 SSL 的 Connection 类
        // 当前 Connection 类不支持 SSL，所以这里只是演示框架

        // 发送欢迎消息
        std::string welcome = "WELCOME (SSL)\n";
        co_await asio::async_write(ssl_socket, asio::buffer(welcome), asio::use_awaitable);

        // 简单的读写循环
        std::vector<char> buffer(4096);
        while (true) {
            size_t n = co_await ssl_socket.async_read_some(
                asio::buffer(buffer),
                asio::use_awaitable
            );

            std::string msg(buffer.data(), n);
            std::cout << "SSL Connection " << conn_id << " received: " << msg;

            if (msg.find("PING") != std::string::npos) {
                co_await asio::async_write(
                    ssl_socket,
                    asio::buffer("PONG\n"),
                    asio::use_awaitable
                );
            } else if (msg.find("QUIT") != std::string::npos) {
                co_await asio::async_write(
                    ssl_socket,
                    asio::buffer("BYE\n"),
                    asio::use_awaitable
                );
                break;
            } else {
                std::string echo = "ECHO: " + msg;
                co_await asio::async_write(
                    ssl_socket,
                    asio::buffer(echo),
                    asio::use_awaitable
                );
            }
        }

        // 优雅关闭 SSL 连接
        co_await ssl_socket.async_shutdown(asio::use_awaitable);

    } catch (const std::exception& e) {
        std::cerr << "SSL Connection " << conn_id << " error: " << e.what() << std::endl;
    }

    std::cout << "SSL Connection " << conn_id << " closed" << std::endl;
}

} // namespace gateway
