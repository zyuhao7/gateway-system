/**
 * @file stress_test_client.cpp
 * @brief 百万连接压测客户端
 *
 * 功能：
 * - 建立大量并发连接
 * - 发送心跳保持连接活跃
 * - 统计连接成功率、延迟、吞吐量
 */

#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <iomanip>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class StressTestClient {
public:
    StressTestClient(asio::io_context& io_context,
                     const std::string& host,
                     uint16_t port,
                     size_t target_connections)
        : io_context_(io_context)
        , host_(host)
        , port_(port)
        , target_connections_(target_connections)
        , connected_count_(0)
        , failed_count_(0)
        , messages_sent_(0)
        , messages_received_(0)
    {}

    void start() {
        std::cout << "Starting stress test..." << std::endl;
        std::cout << "Target: " << target_connections_ << " connections" << std::endl;
        std::cout << "Server: " << host_ << ":" << port_ << std::endl;
        std::cout << std::endl;

        start_time_ = std::chrono::steady_clock::now();

        // 分批建立连接，避免瞬间压力过大
        const size_t batch_size = 1000;
        for (size_t i = 0; i < target_connections_; i += batch_size) {
            size_t count = std::min(batch_size, target_connections_ - i);
            for (size_t j = 0; j < count; j++) {
                create_connection();
            }

            // 每批次之间短暂延迟
            if (i + batch_size < target_connections_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void print_stats() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

        std::cout << "\n=== Stress Test Statistics ===" << std::endl;
        std::cout << "Elapsed time:       " << elapsed << " seconds" << std::endl;
        std::cout << "Target connections: " << target_connections_ << std::endl;
        std::cout << "Connected:          " << connected_count_.load() << std::endl;
        std::cout << "Failed:             " << failed_count_.load() << std::endl;
        std::cout << "Success rate:       " << std::fixed << std::setprecision(2)
                  << (100.0 * connected_count_.load() / target_connections_) << "%" << std::endl;
        std::cout << "Messages sent:      " << messages_sent_.load() << std::endl;
        std::cout << "Messages received:  " << messages_received_.load() << std::endl;

        if (elapsed > 0) {
            std::cout << "Throughput:         " << (messages_sent_.load() / elapsed) << " msg/s" << std::endl;
        }
        std::cout << std::endl;
    }

private:
    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        Connection(asio::io_context& io_context,
                   StressTestClient* client)
            : socket_(io_context)
            , client_(client)
            , heartbeat_timer_(io_context)
        {}

        void connect(const std::string& host, uint16_t port) {
            auto self = shared_from_this();

            tcp::resolver resolver(socket_.get_executor());
            auto endpoints = resolver.resolve(host, std::to_string(port));

            asio::async_connect(socket_, endpoints,
                [this, self](const boost::system::error_code& ec, const tcp::endpoint&) {
                    if (!ec) {
                        client_->connected_count_++;
                        start_read();
                        start_heartbeat();
                    } else {
                        client_->failed_count_++;
                    }
                });
        }

    private:
        void start_read() {
            auto self = shared_from_this();

            asio::async_read_until(socket_, buffer_, '\n',
                [this, self](const boost::system::error_code& ec, size_t) {
                    if (!ec) {
                        client_->messages_received_++;
                        buffer_.consume(buffer_.size());
                        start_read();
                    }
                });
        }

        void start_heartbeat() {
            auto self = shared_from_this();

            heartbeat_timer_.expires_after(std::chrono::seconds(30));
            heartbeat_timer_.async_wait(
                [this, self](const boost::system::error_code& ec) {
                    if (!ec) {
                        send_ping();
                        start_heartbeat();
                    }
                });
        }

        void send_ping() {
            auto self = shared_from_this();

            auto message = std::make_shared<std::string>("PING\n");
            asio::async_write(socket_, asio::buffer(*message),
                [this, self, message](const boost::system::error_code& ec, size_t) {
                    if (!ec) {
                        client_->messages_sent_++;
                    }
                });
        }

        tcp::socket socket_;
        StressTestClient* client_;
        asio::steady_timer heartbeat_timer_;
        asio::streambuf buffer_;
    };

    void create_connection() {
        auto conn = std::make_shared<Connection>(io_context_, this);
        conn->connect(host_, port_);
        connections_.push_back(conn);
    }

    asio::io_context& io_context_;
    std::string host_;
    uint16_t port_;
    size_t target_connections_;

    std::vector<std::shared_ptr<Connection>> connections_;
    std::atomic<size_t> connected_count_;
    std::atomic<size_t> failed_count_;
    std::atomic<size_t> messages_sent_;
    std::atomic<size_t> messages_received_;

    std::chrono::steady_clock::time_point start_time_;
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <connections>" << std::endl;
        std::cerr << "Example: " << argv[0] << " localhost 8080 10000" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    uint16_t port = std::stoi(argv[2]);
    size_t connections = std::stoull(argv[3]);

    try {
        // 使用多个 io_context 提高并发性能
        const size_t num_threads = std::thread::hardware_concurrency();
        std::vector<asio::io_context> io_contexts(num_threads);
        std::vector<std::thread> threads;

        std::cout << "Using " << num_threads << " threads" << std::endl;

        // 创建客户端（分配到不同的 io_context）
        std::vector<std::unique_ptr<StressTestClient>> clients;
        size_t connections_per_context = connections / num_threads;

        for (size_t i = 0; i < num_threads; i++) {
            size_t count = (i == num_threads - 1)
                ? (connections - i * connections_per_context)
                : connections_per_context;

            clients.push_back(std::make_unique<StressTestClient>(
                io_contexts[i], host, port, count));
        }

        // 启动所有客户端
        for (auto& client : clients) {
            client->start();
        }

        // 启动 io_context 线程
        for (size_t i = 0; i < num_threads; i++) {
            threads.emplace_back([&io_contexts, i]() {
                io_contexts[i].run();
            });
        }

        // 定期打印统计信息
        for (int i = 0; i < 60; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            std::cout << "\n--- Progress Report (" << (i + 1) * 5 << "s) ---" << std::endl;
            for (auto& client : clients) {
                client->print_stats();
            }
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
