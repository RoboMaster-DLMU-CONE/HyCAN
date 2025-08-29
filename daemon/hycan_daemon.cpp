#include "HyCAN/Interface/Daemon.hpp"

#include <iostream>
#include <csignal>
#include <memory>

static std::unique_ptr<HyCAN::Daemon> g_daemon;

void signal_handler(int signal)
{
    std::cout << "\nReceived signal " << signal << ", shutting down daemon..." << std::endl;
    if (g_daemon)
    {
        g_daemon->stop();
    }
}

int main(int argc, char* argv[])
{
    std::string socket_path = "/tmp/hycan_daemon.sock";
    
    // Parse command line arguments
    if (argc > 1)
    {
        socket_path = argv[1];
    }

    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "Starting HyCAN daemon on socket: " << socket_path << std::endl;

    try
    {
        g_daemon = std::make_unique<HyCAN::Daemon>();
        
        auto result = g_daemon->start(socket_path);
        if (!result)
        {
            std::cerr << "Failed to start daemon: " << result.error().message << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in daemon: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "HyCAN daemon stopped." << std::endl;
    return 0;
}