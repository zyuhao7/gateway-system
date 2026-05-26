/**
 * @file test_ssl_server.cpp
 * @brief 测试 SSL/TLS 服务器
 */

#include "ssl_server.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>

namespace asio = boost::asio;

int main() {
    std::cout << "=== SSL/TLS Server Test ===" << std::endl;

    try {
        asio::io_context io_context;

        // 启动 SSL 服务器
        std::cout << "\n[Test 1] Starting SSL server on port 8443..." << std::endl;
        gateway::SslServer server(
            io_context,
            8443,
            "certs/server.crt",
            "certs/server.key"
        );

        // 在后台线程启动服务器
        std::thread server_thread([&]() {
            asio::co_spawn(io_context, server.listen(), asio::detached);
            io_context.run();
        });

        std::cout << "✓ SSL server started" << std::endl;

        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "\n[Test 2] Server is running..." << std::endl;
        std::cout << "You can test with:" << std::endl;
        std::cout << "  openssl s_client -connect localhost:8443" << std::endl;
        std::cout << "  Then type: PING" << std::endl;
        std::cout << "\nPress Ctrl+C to stop the server" << std::endl;

        // 等待用户中断
        server_thread.join();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
