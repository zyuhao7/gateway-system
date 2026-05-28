#include "server.hpp"
#include "websocket_server.hpp"
#include "io_context_pool.hpp"
#include "metrics_server.hpp"
#include <iostream>
#include <csignal>
#include <thread>

int main(int argc, char* argv[]) {
    try {
        uint16_t tcp_port = 8080;
        uint16_t ws_port = 8081;
        uint16_t metrics_port = 9090;
        size_t thread_count = 1;

        if (argc > 1) {
            tcp_port = static_cast<uint16_t>(std::stoi(argv[1]));
        }
        if (argc > 2) {
            ws_port = static_cast<uint16_t>(std::stoi(argv[2]));
        }
        if (argc > 3) {
            thread_count = std::stoul(argv[3]);
        }
        if (argc > 4) {
            metrics_port = static_cast<uint16_t>(std::stoi(argv[4]));
        }

        std::cout << "Starting Gateway System" << std::endl;
        std::cout << "TCP Port: " << tcp_port << std::endl;
        std::cout << "WebSocket Port: " << ws_port << std::endl;
        std::cout << "Metrics Port: " << metrics_port << std::endl;
        std::cout << "Threads: " << thread_count << std::endl;
        std::cout << std::endl;

        if (thread_count == 1) {
            // Single-threaded mode
            boost::asio::io_context io_context;

            boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto) {
                std::cout << "\nShutting down..." << std::endl;
                io_context.stop();
            });

            gateway::Server tcp_server(io_context, tcp_port);
            gateway::WebSocketServer ws_server(io_context, ws_port);
            MetricsServer metrics_server(io_context, metrics_port);

            boost::asio::co_spawn(io_context,
                                 tcp_server.listen(),
                                 boost::asio::detached);

            boost::asio::co_spawn(io_context,
                                 ws_server.listen(),
                                 boost::asio::detached);

            boost::asio::co_spawn(io_context,
                                 metrics_server.listen(),
                                 boost::asio::detached);

            io_context.run();
        } else {
            // Multi-threaded mode
            boost::asio::io_context acceptor_io_context;
            auto pool = std::make_shared<IoContextPool>(thread_count);

            boost::asio::signal_set signals(acceptor_io_context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto) {
                std::cout << "\nShutting down..." << std::endl;
                acceptor_io_context.stop();
                pool->stop();
            });

            gateway::Server tcp_server(acceptor_io_context, pool, tcp_port);
            gateway::WebSocketServer ws_server(acceptor_io_context, pool, ws_port);
            MetricsServer metrics_server(acceptor_io_context, metrics_port);

            boost::asio::co_spawn(acceptor_io_context,
                                 tcp_server.listen(),
                                 boost::asio::detached);

            boost::asio::co_spawn(acceptor_io_context,
                                 ws_server.listen(),
                                 boost::asio::detached);

            boost::asio::co_spawn(acceptor_io_context,
                                 metrics_server.listen(),
                                 boost::asio::detached);

            pool->run();

            acceptor_io_context.run();
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
