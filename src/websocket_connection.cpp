/**
 * @file websocket_connection.cpp
 * @brief WebSocket 连接实现
 */

#include "websocket_connection.hpp"
#include "connection_manager.hpp"
#include "metrics.hpp"
#include <iostream>
#include <boost/json.hpp>

namespace gateway {

namespace json = boost::json;

WebSocketConnection::WebSocketConnection(tcp::socket socket, uint64_t id)
    : ws_(std::move(socket))
    , id_(id)
    , heartbeat_timer_(ws_.get_executor())
    , last_activity_(std::chrono::steady_clock::now()) {
}

asio::awaitable<void> WebSocketConnection::start() {
    try {
        // WebSocket 握手
        co_await ws_.async_accept(asio::use_awaitable);

        // 发送欢迎消息
        json::object welcome;
        welcome["type"] = "welcome";
        welcome["connection_id"] = id_;
        welcome["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        co_await send_message(json::serialize(welcome));

        // 启动心跳检测
        asio::co_spawn(ws_.get_executor(),
                      heartbeat_loop(),
                      asio::detached);

        // 启动读取循环
        co_await read_loop();

    } catch (const std::exception& e) {
        std::cerr << "WebSocket connection " << id_ << " error: " << e.what() << std::endl;
    }
}

asio::awaitable<void> WebSocketConnection::read_loop() {
    beast::flat_buffer buffer;

    while (true) {
        try {
            // 读取消息
            co_await ws_.async_read(buffer, asio::use_awaitable);

            last_activity_ = std::chrono::steady_clock::now();

            // 解析消息
            std::string msg = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());

            Metrics::instance().message_received();

            // 处理消息
            co_await handle_message(msg);

        } catch (const beast::system_error& e) {
            if (e.code() == websocket::error::closed) {
                std::cout << "WebSocket " << id_ << " closed normally" << std::endl;
                break;
            }
            throw;
        }
    }
}

asio::awaitable<void> WebSocketConnection::handle_message(const std::string& msg) {
    boost::system::error_code ec;

    // 尝试解析 JSON
    auto jv = json::parse(msg, ec);

    if (ec) {
        // JSON 解析失败，当作文本消息处理
        std::cout << "WebSocket " << id_ << " received text: " << msg << std::endl;

        json::object response;
        response["type"] = "echo";
        response["content"] = msg;
        co_await send_message(json::serialize(response));
        co_return;
    }

    auto& obj = jv.as_object();

    std::string type = obj.contains("type") ?
        std::string(obj["type"].as_string()) : "unknown";

    if (type == "ping") {
        // 响应 PONG
        json::object pong;
        pong["type"] = "pong";
        pong["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        co_await send_message(json::serialize(pong));

    } else if (type == "message") {
        // 回显消息
        json::object response;
        response["type"] = "message";
        response["content"] = obj.contains("content") ?
            obj["content"].as_string() : "";
        response["echo"] = true;
        co_await send_message(json::serialize(response));

    } else if (type == "broadcast") {
        // 广播消息
        std::string content = obj.contains("content") ?
            std::string(obj["content"].as_string()) : "";
        ConnectionManager::instance().broadcast(content);

        json::object ack;
        ack["type"] = "ack";
        ack["status"] = "broadcast_sent";
        co_await send_message(json::serialize(ack));

    } else if (type == "stats") {
        // 返回统计信息
        auto total = ConnectionManager::instance().total_count();
        auto active = ConnectionManager::instance().active_count();

        json::object stats;
        stats["type"] = "stats";
        stats["total_connections"] = total;
        stats["active_connections"] = active;
        co_await send_message(json::serialize(stats));

    } else {
        // 未知消息类型
        json::object error;
        error["type"] = "error";
        error["message"] = "Unknown message type: " + type;
        co_await send_message(json::serialize(error));
    }
}

asio::awaitable<void> WebSocketConnection::heartbeat_loop() {
    while (true) {
        try {
            heartbeat_timer_.expires_after(heartbeat_interval_);
            co_await heartbeat_timer_.async_wait(asio::use_awaitable);

            auto now = std::chrono::steady_clock::now();
            auto idle = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_);

            if (idle > timeout_) {
                std::cout << "WebSocket " << id_ << " timeout, closing" << std::endl;
                close();
                break;
            }

        } catch (const std::exception& e) {
            break;
        }
    }
}

asio::awaitable<void> WebSocketConnection::send_message(const std::string& msg) {
    co_await ws_.async_write(
        asio::buffer(msg),
        asio::use_awaitable
    );

    Metrics::instance().message_sent();
}

asio::awaitable<void> WebSocketConnection::send_binary(const std::vector<uint8_t>& data) {
    ws_.binary(true);

    co_await ws_.async_write(
        asio::buffer(data),
        asio::use_awaitable
    );

    ws_.text(true);

    Metrics::instance().message_sent();
}

void WebSocketConnection::close() {
    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
}

}
