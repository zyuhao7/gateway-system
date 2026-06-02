/**
 * @file stress_test.cpp
 * @brief 高并发压力测试工具
 */

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

using boost::asio::ip::tcp;

class StressTest {
public:
    StressTest(const std::string& host, uint16_t port, int target_connections)
        : host_(host)
        , port_(port)
        , target_connections_(target_connections)
        , connected_(0)
        , failed_(0)
        , messages_sent_(0)
        , messages_received_(0) {
    }

    void run() {
        std::cout << "=== Gateway Stress Test ===" << std::endl;
        std::cout << "Target: " << host_ << ":" << port_ << std::endl;
        std::cout << "Connections: " << target_connections_ << std::endl;
        std::cout << std::endl;

        auto start = std::chrono::steady_clock::now();

        // 启动连接线程
        std::vector<std::thread> threads;
        int threads_count = std::min(32, target_connections_ / 100);
        int connections_per_thread = target_connections_ / threads_count;

        std::cout << "Starting " << threads_count << " threads..." << std::endl;

        for (int i = 0; i < threads_count; ++i) {
            int count = (i == threads_count - 1)
                ? target_connections_ - (connections_per_thread * (threads_count - 1))
                : connections_per_thread;

            threads.emplace_back([this, count]() {
                this->connect_loop(count);
            });
        }

        // 等待所有连接建立
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cout << "Connected: " << connected_ << std::endl;
        std::cout << "Failed: " << failed_ << std::endl;

        // 发送消息测试
        std::cout << "\nSending messages..." << std::endl;
        running_ = true;

        // 运行60秒
        for (int i = 0; i < 60; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Time: " << (i+1) << "s, "
                     << "Sent: " << messages_sent_ << ", "
                     << "Recv: " << messages_received_ << std::endl;
        }

        running_ = false;

        // 等待线程结束
        for (auto& t : threads) {
            t.join();
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        // 输出结果
        std::cout << "\n=== Test Results ===" << std::endl;
        std::cout << "Duration: " << duration << "s" << std::endl;
        std::cout << "Total Connected: " << connected_ << std::endl;
        std::cout << "Total Failed: " << failed_ << std::endl;
        std::cout << "Messages Sent: " << messages_sent_ << std::endl;
        std::cout << "Messages Received: " << messages_received_ << std::endl;
        std::cout << "Throughput: " << (messages_sent_ / duration) << " msg/s" << std::endl;
    }

private:
    void connect_loop(int count) {
        boost::asio::io_context io_context;

        for (int i = 0; i < count; ++i) {
            try {
                auto socket = std::make_shared<tcp::socket>(io_context);

                tcp::resolver resolver(io_context);
                auto endpoints = resolver.resolve(host_, std::to_string(port_));

                boost::asio::connect(*socket, endpoints);
                connected_++;

                // 发送和接收消息
                while (running_) {
                    try {
                        // 发送PING
                        std::string msg = "PING\n";
                        boost::asio::write(*socket, boost::asio::buffer(msg));
                        messages_sent_++;

                        // 接收响应
                        boost::asio::streambuf buffer;
                        boost::asio::read_until(*socket, buffer, '\n');
                        messages_received_++;

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    } catch (...) {
                        break;
                    }
                }

                socket->close();

            } catch (const std::exception& e) {
                failed_++;
            }
        }
    }

    std::string host_;
    uint16_t port_;
    int target_connections_;
    std::atomic<int> connected_;
    std::atomic<int> failed_;
    std::atomic<int> messages_sent_;
    std::atomic<int> messages_received_;
    std::atomic<bool> running_{false};
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <host> <port> [connections]" << std::endl;
        std::cout << "Example: " << argv[0] << " localhost 8080 20000" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    uint16_t port = std::stoi(argv[2]);
    int connections = (argc > 3) ? std::stoi(argv[3]) : 10000;

    StressTest test(host, port, connections);
    test.run();

    return 0;
}
