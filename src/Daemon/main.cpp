#include <csignal>
#include <iostream>

#include "HyCAN/Daemon/Daemon.hpp"

static HyCAN::Daemon* g_daemon_instance = nullptr;

void signal_handler(int sig)
{
    if (g_daemon_instance && (sig == SIGTERM || sig == SIGINT))
    {
        std::cout << "Received signal " << sig << ", stopping daemon..." << std::endl;
        g_daemon_instance->stop();
    }
}

int main()
{
    try
    {
        HyCAN::Daemon daemon;
        g_daemon_instance = &daemon;

        // 设置信号处理
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);

        return daemon.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
