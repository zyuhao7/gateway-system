#include "server.hpp"
#include <iostream>
#include <csignal>

int main(int argc, char* argv[]) {
    try {
        uint16_t port = 8080;
        if (argc > 1) {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        }

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

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
