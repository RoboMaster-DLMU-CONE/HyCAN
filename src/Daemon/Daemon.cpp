#include <iostream>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <format>
#include <net/if.h>

#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>

#include "libipc/ipc.h"
#include "HyCAN/Daemon/Daemon.hpp"
#include "HyCAN/Daemon/DaemonClass.hpp"
#include "HyCAN/Daemon/VCAN.hpp"

namespace HyCAN
{
    Daemon::Daemon()
    {
        if (init_netlink() < 0)
        {
            throw std::runtime_error("Failed to initialize netlink in daemon");
        }
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

                std::cout << "Processing request for interface: " << request->interface_name
                    << ", action: " << (request->up ? "UP" : "DOWN") << std::endl;

                // 处理请求
                auto response = process_request(*request);

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
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
        }
    }

    NetlinkResponse Daemon::set_interface_state_libnl(std::string_view interface_name, const bool up) const
    {
        // 刷新缓存以获取最新状态
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, "Failed to refresh link cache");
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        if (!link)
        {
            return NetlinkResponse(-1, std::format("Interface {} not found", interface_name));
        }

        rtnl_link* change = rtnl_link_alloc();
        if (!change)
        {
            rtnl_link_put(link);
            return NetlinkResponse(-1, "Failed to allocate change link object");
        }

        // 设置接口状态
        if (up)
        {
            rtnl_link_set_flags(change, IFF_UP);
        }
        else
        {
            rtnl_link_unset_flags(change, IFF_UP);
        }

        const int result = rtnl_link_change(nl_socket_, link, change, 0);

        rtnl_link_put(change);
        rtnl_link_put(link);

        if (result < 0)
        {
            return NetlinkResponse(result, std::format("rtnl_link_change failed: {}", nl_geterror(result)));
        }

        // 再次刷新缓存以反映更改
        nl_cache_refill(nl_socket_, link_cache_);

        return NetlinkResponse(0, "Success");
    }

    NetlinkResponse Daemon::set_can_bitrate_libnl(std::string_view interface_name, const uint32_t bitrate) const
    {
        if (!interface_name.starts_with("can"))
        {
            return NetlinkResponse(0, "Not a CAN interface, skipping bitrate setting");
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        if (!link)
        {
            return NetlinkResponse(-1, std::format("CAN Interface {} not found", interface_name));
        }

        rtnl_link* change = rtnl_link_alloc();
        if (!change)
        {
            rtnl_link_put(link);
            return NetlinkResponse(-1, "Failed to allocate change link object for CAN");
        }

        // 设置 CAN 比特率
        if (rtnl_link_is_can(link))
        {
            rtnl_link_can_set_bitrate(change, bitrate);
            const int result = rtnl_link_change(nl_socket_, link, change, 0);

            rtnl_link_put(change);
            rtnl_link_put(link);

            if (result < 0)
            {
                return NetlinkResponse(result, std::format("Failed to set CAN bitrate: {}", nl_geterror(result)));
            }

            return NetlinkResponse(0, "CAN bitrate set successfully");
        }
        else
        {
            rtnl_link_put(change);
            rtnl_link_put(link);
            return NetlinkResponse(-1, "Interface is not a CAN interface");
        }
    }

    NetlinkResponse Daemon::check_interface_state_libnl(std::string_view interface_name) const
    {
        // 刷新缓存以获取最新状态
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, false, "Failed to refresh link cache");
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        if (!link)
        {
            return NetlinkResponse(-1, false, std::format("Interface {} not found", interface_name));
        }

        const unsigned int flags = rtnl_link_get_flags(link);
        const bool is_up = (flags & IFF_UP) != 0;

        rtnl_link_put(link);

        return NetlinkResponse(0, is_up, "Success");
    }

    NetlinkResponse Daemon::validate_can_hardware_libnl(std::string_view interface_name) const
    {
        // 刷新缓存以获取最新状态
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, false, true, "Failed to refresh link cache");
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        if (!link)
        {
            return NetlinkResponse(-1, false, true, std::format("CAN interface {} not found", interface_name));
        }

        const bool is_can = rtnl_link_is_can(link);

        rtnl_link_put(link);

        if (!is_can)
        {
            return NetlinkResponse(-1, false, true, std::format("Interface {} is not a CAN interface", interface_name));
        }

        return NetlinkResponse(0, true, true, "CAN hardware interface found");
    }

    NetlinkResponse Daemon::process_request(const NetlinkRequest& request) const
    {
        switch (request.operation)
        {
            case RequestType::SET_INTERFACE_STATE:
            {
                // Create VCAN interface if needed
                if (request.create_vcan_if_needed)
                {
                    auto vcan_result = create_vcan_interface_if_not_exists(request.interface_name);
                    if (!vcan_result)
                    {
                        return NetlinkResponse(-1, std::format("Failed to create VCAN interface {}: {}",
                                                               request.interface_name, vcan_result.error().message));
                    }
                }

                // 如果需要设置比特率（仅对 CAN 接口）
                if (request.up && request.set_bitrate)
                {
                    const auto bitrate_result = set_can_bitrate_libnl(request.interface_name, request.bitrate);
                    if (bitrate_result.result != 0)
                    {
                        return bitrate_result;
                    }
                }

                // 设置接口状态
                return set_interface_state_libnl(request.interface_name, request.up);
            }
            case RequestType::CHECK_INTERFACE_STATE:
                return check_interface_state_libnl(request.interface_name);
            case RequestType::VALIDATE_CAN_HARDWARE:
                return validate_can_hardware_libnl(request.interface_name);
            case RequestType::CREATE_VCAN_INTERFACE:
            {
                auto vcan_result = create_vcan_interface_if_not_exists(request.interface_name);
                if (!vcan_result)
                {
                    return NetlinkResponse(-1, std::format("Failed to create VCAN interface {}: {}",
                                                           request.interface_name, vcan_result.error().message));
                }
                return NetlinkResponse(0, "VCAN interface created successfully");
            }
            default:
                return NetlinkResponse(-1, "Unknown request operation");
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

        // 设置信号处理
        signal(SIGTERM, HyCAN::signal_handler);
        signal(SIGINT, HyCAN::signal_handler);

        return daemon.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
