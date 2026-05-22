#include "server.hpp"
#include "io_context_pool.hpp"
#include <iostream>
#include <csignal>
#include <thread>

int main(int argc, char* argv[]) {
    try {
        uint16_t port = 8080;
        size_t thread_count = 1;

        if (argc > 1) {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        }
        if (argc > 2) {
            thread_count = std::stoul(argv[2]);
        }

        if (thread_count == 1) {
            // Single-threaded mode
            boost::asio::io_context io_context;

            boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto) {
                std::cout << "\nShutting down..." << std::endl;
                io_context.stop();
            });

            gateway::Server server(io_context, port);

            boost::asio::co_spawn(io_context,
                                 server.listen(),
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

            gateway::Server server(acceptor_io_context, pool, port);

            boost::asio::co_spawn(acceptor_io_context,
                                 server.listen(),
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
