/**
 * @file metrics_server.hpp
 * @brief HTTP 服务器，暴露 Prometheus 指标
 *
 * 提供 /metrics 端点供 Prometheus 抓取
 */

#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

/**
 * @class MetricsServer
 * @brief 简单的 HTTP 服务器，用于暴露指标
 *
 * 监听指定端口，响应 GET /metrics 请求
 */
class MetricsServer {
public:
    explicit MetricsServer(asio::io_context& io_context, uint16_t port);

    asio::awaitable<void> listen();

private:
    asio::awaitable<void> handle_client(tcp::socket socket);
    std::string handle_request(const std::string& request);

    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};
