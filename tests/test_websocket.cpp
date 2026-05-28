/**
 * @file test_websocket.cpp
 * @brief WebSocket 功能测试
 */

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <string>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace json = boost::json;
using tcp = asio::ip::tcp;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " localhost 8080" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    std::string port = argv[2];

    try {
        asio::io_context ioc;

        // 解析地址
        tcp::resolver resolver(ioc);
        auto results = resolver.resolve(host, port);

        // 连接
        websocket::stream<tcp::socket> ws(ioc);
        asio::connect(ws.next_layer(), results);

        // WebSocket 握手
        ws.handshake(host, "/");

        std::cout << "Connected to " << host << ":" << port << std::endl;

        // 读取欢迎消息
        beast::flat_buffer buffer;
        ws.read(buffer);
        std::string welcome = beast::buffers_to_string(buffer.data());
        std::cout << "Received: " << welcome << std::endl;
        buffer.consume(buffer.size());

        // 测试 1: PING
        std::cout << "\n[Test 1] Sending PING..." << std::endl;
        json::object ping_msg;
        ping_msg["type"] = "ping";
        ws.write(asio::buffer(json::serialize(ping_msg)));

        ws.read(buffer);
        std::string pong = beast::buffers_to_string(buffer.data());
        std::cout << "Received: " << pong << std::endl;
        buffer.consume(buffer.size());

        // 测试 2: 发送消息
        std::cout << "\n[Test 2] Sending message..." << std::endl;
        json::object msg;
        msg["type"] = "message";
        msg["content"] = "Hello, WebSocket!";
        ws.write(asio::buffer(json::serialize(msg)));

        ws.read(buffer);
        std::string echo = beast::buffers_to_string(buffer.data());
        std::cout << "Received: " << echo << std::endl;
        buffer.consume(buffer.size());

        // 测试 3: 获取统计信息
        std::cout << "\n[Test 3] Requesting stats..." << std::endl;
        json::object stats_msg;
        stats_msg["type"] = "stats";
        ws.write(asio::buffer(json::serialize(stats_msg)));

        ws.read(buffer);
        std::string stats = beast::buffers_to_string(buffer.data());
        std::cout << "Received: " << stats << std::endl;
        buffer.consume(buffer.size());

        // 测试 4: 广播消息
        std::cout << "\n[Test 4] Broadcasting message..." << std::endl;
        json::object broadcast_msg;
        broadcast_msg["type"] = "broadcast";
        broadcast_msg["content"] = "Broadcast from WebSocket client";
        ws.write(asio::buffer(json::serialize(broadcast_msg)));

        ws.read(buffer);
        std::string ack = beast::buffers_to_string(buffer.data());
        std::cout << "Received: " << ack << std::endl;
        buffer.consume(buffer.size());

        // 关闭连接
        std::cout << "\n[Test 5] Closing connection..." << std::endl;
        ws.close(websocket::close_code::normal);

        std::cout << "\n=== All tests passed ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
