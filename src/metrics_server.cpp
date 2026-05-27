/**
 * @file metrics_server.cpp
 * @brief Prometheus 指标 HTTP 服务器实现
 */

#include "metrics_server.hpp"
#include "metrics.hpp"
#include "buffer_pool.hpp"
#include <iostream>
#include <sstream>

MetricsServer::MetricsServer(asio::io_context& io_context, uint16_t port)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "Metrics server listening on port " << port << std::endl;
}

asio::awaitable<void> MetricsServer::listen() {
    while (true) {
        tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);

        asio::co_spawn(io_context_,
                      handle_client(std::move(socket)),
                      asio::detached);
    }
}

asio::awaitable<void> MetricsServer::handle_client(tcp::socket socket) {
    try {
        // 读取 HTTP 请求
        std::vector<char> buffer(4096);
        size_t n = co_await socket.async_read_some(
            asio::buffer(buffer),
            asio::use_awaitable
        );

        std::string request(buffer.data(), n);

        // 处理请求并生成响应
        std::string response = handle_request(request);

        // 发送响应
        co_await asio::async_write(
            socket,
            asio::buffer(response),
            asio::use_awaitable
        );

    } catch (const std::exception& e) {
        std::cerr << "Metrics server error: " << e.what() << std::endl;
    }
}

/**
 * 处理 HTTP 请求
 *
 * 支持：
 * - GET /metrics - 返回 Prometheus 格式的指标
 * - 其他路径 - 返回 404
 */
std::string MetricsServer::handle_request(const std::string& request) {
    std::ostringstream response;

    // 简单的 HTTP 请求解析
    if (request.find("GET /metrics") != std::string::npos) {
        // 更新缓冲池统计
        auto& pool = BufferPool::instance();
        Metrics::instance().record_buffer_pool_stats(
            pool.allocated(),
            pool.in_use(),
            pool.available()
        );

        // 获取 Prometheus 格式的指标
        std::string metrics = Metrics::instance().export_prometheus();

        // 构造 HTTP 响应
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain; version=0.0.4\r\n";
        response << "Content-Length: " << metrics.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << metrics;

    } else {
        // 404 Not Found
        std::string body = "404 Not Found\n";
        response << "HTTP/1.1 404 Not Found\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
    }

    return response.str();
}
