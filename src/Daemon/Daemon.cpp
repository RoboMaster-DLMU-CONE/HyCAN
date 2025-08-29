#include <iostream>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <format>
#include <memory>

#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>

#include "libipc/ipc.h"
#include "HyCAN/Daemon/Daemon.hpp"
#include "HyCAN/Daemon/DaemonClass.hpp"
#include "HyCAN/Daemon/InterfaceManager.hpp"
#include "HyCAN/Daemon/CANManager.hpp"
#include "HyCAN/Daemon/RequestProcessor.hpp"

namespace HyCAN
{
    Daemon::Daemon()
    {
        if (init_netlink() < 0)
        {
            throw std::runtime_error("Failed to initialize netlink in daemon");
        }

        // Initialize modular components
        interface_manager_ = std::make_unique<InterfaceManager>(nl_socket_, link_cache_);
        can_manager_ = std::make_unique<CANManager>(nl_socket_, link_cache_);
        request_processor_ = std::make_unique<RequestProcessor>(*interface_manager_, *can_manager_);
    }

    Daemon::~Daemon()
    {
        stop();
        cleanup_netlink();
    }

    int Daemon::run()
    {
        if (geteuid() != 0)
        {
            std::cerr << "HyCAN daemon must run as root" << std::endl;
            return 1;
        }

        std::cout << "HyCAN Daemon started, listening on channel 'HyCAN_Daemon'..." << std::endl;

        while (running_.load(std::memory_order_acquire))
        {
            try
            {
                // 等待客户端请求，超时 1 秒以便检查停止标志
                auto received = channel_.recv(1000); // 1000ms 超时

                if (received.empty())
                {
                    continue; // 超时，继续检查停止标志
                }

                // 解析请求
                if (received.size() != sizeof(NetlinkRequest))
                {
                    std::cerr << "Received invalid request size: " << received.size() << std::endl;
                    continue;
                }

                const auto* request = static_cast<const NetlinkRequest*>(received.data());

                const char* operation_name = "UNKNOWN";
                switch (request->operation)
                {
                    case RequestType::SET_INTERFACE_STATE:
                        operation_name = request->up ? "UP" : "DOWN";
                        break;
                    case RequestType::CHECK_INTERFACE_STATE:
                        operation_name = "CHECK_STATE";
                        break;
                    case RequestType::VALIDATE_CAN_HARDWARE:
                        operation_name = "VALIDATE_CAN";
                        break;
                    case RequestType::CREATE_VCAN_INTERFACE:
                        operation_name = "CREATE_VCAN";
                        break;
                }

                std::cout << "Processing request for interface: " << request->interface_name
                    << ", operation: " << operation_name << std::endl;

                // 处理请求
                auto response = request_processor_->process_request(*request);

                // 发送响应
                auto response_data = ipc::buff_t{reinterpret_cast<ipc::byte_t*>(&response), sizeof(response)};
                channel_.send(response_data);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception in daemon main loop: " << e.what() << std::endl;

                // 发送错误响应
                NetlinkResponse error_response(-1, std::format("Daemon exception: {}", e.what()));
                auto error_data = ipc::buff_t{
                    reinterpret_cast<ipc::byte_t*>(&error_response), sizeof(error_response)
                };
                try
                {
                    channel_.send(error_data);
                }
                catch (...)
                {
                    // 忽略发送错误
                }
            }
        }

        std::cout << "HyCAN Daemon stopped." << std::endl;
        return 0;
    }

    void Daemon::stop()
    {
        running_.store(false, std::memory_order_release);
    }

    int Daemon::init_netlink()
    {
        nl_socket_ = nl_socket_alloc();
        if (!nl_socket_)
        {
            std::cerr << "Failed to allocate netlink socket" << std::endl;
            return -1;
        }

        if (nl_connect(nl_socket_, NETLINK_ROUTE) < 0)
        {
            std::cerr << "Failed to connect to netlink" << std::endl;
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
            return -1;
        }

        if (rtnl_link_alloc_cache(nl_socket_, AF_UNSPEC, &link_cache_) < 0)
        {
            std::cerr << "Failed to allocate link cache" << std::endl;
            nl_close(nl_socket_);
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
            return -1;
        }

        return 0;
    }

    void Daemon::cleanup_netlink()
    {
        if (link_cache_)
        {
            nl_cache_free(link_cache_);
            link_cache_ = nullptr;
        }

        if (nl_socket_)
        {
            nl_close(nl_socket_);
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
        }
    }

    // 静态实例用于信号处理
    static Daemon* g_daemon_instance = nullptr;

    void signal_handler(int sig)
    {
        if (g_daemon_instance && (sig == SIGTERM || sig == SIGINT))
        {
            std::cout << "Received signal " << sig << ", stopping daemon..." << std::endl;
            g_daemon_instance->stop();
        }
    }
} // namespace HyCAN

// 主函数实现
int main()
{
    try
    {
        HyCAN::Daemon daemon;
        HyCAN::g_daemon_instance = &daemon;

        // 注册信号处理器
        std::signal(SIGTERM, HyCAN::signal_handler);
        std::signal(SIGINT, HyCAN::signal_handler);

        return daemon.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Daemon initialization failed: " << e.what() << std::endl;
        return 1;
    }
}