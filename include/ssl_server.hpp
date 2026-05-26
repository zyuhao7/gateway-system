/**
 * @file ssl_server.hpp
 * @brief TLS/SSL 支持的服务器
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <string>

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// Forward declarations
class IoContextPool;

namespace gateway {

/**
 * @class SslServer
 * @brief 支持 TLS/SSL 的网关服务器
 */
class SslServer {
public:
    /**
     * @brief 构造函数（单线程模式）
     * @param io_context IO 上下文
     * @param port 监听端口
     * @param cert_file 证书文件路径
     * @param key_file 私钥文件路径
     */
    SslServer(asio::io_context& io_context,
             uint16_t port,
             const std::string& cert_file,
             const std::string& key_file);

    /**
     * @brief 构造函数（多线程模式）
     * @param acceptor_io_context Acceptor IO 上下文
     * @param pool IO 上下文池
     * @param port 监听端口
     * @param cert_file 证书文件路径
     * @param key_file 私钥文件路径
     */
    SslServer(asio::io_context& acceptor_io_context,
             std::shared_ptr<IoContextPool> pool,
             uint16_t port,
             const std::string& cert_file,
             const std::string& key_file);

    /**
     * @brief 启动监听
     */
    asio::awaitable<void> listen();

private:
    /**
     * @brief 处理客户端连接
     */
    asio::awaitable<void> handle_client(ssl::stream<tcp::socket> socket);

    /**
     * @brief 初始化 SSL 上下文
     */
    void init_ssl_context(const std::string& cert_file, const std::string& key_file);

    asio::io_context& acceptor_io_context_;
    std::shared_ptr<IoContextPool> pool_;
    tcp::acceptor acceptor_;
    ssl::context ssl_context_;
    bool use_pool_;
    std::atomic<uint64_t> next_conn_id_{0};
};

} // namespace gateway
