#include <iostream>
#include <format>
#include <net/if.h>

#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>

#include "HyCAN/Daemon/NetlinkManager.hpp"
#include "HyCAN/Daemon/Message.hpp"
#include "HyCAN/Daemon/VCAN.hpp"

namespace HyCAN
{
    NetlinkManager::NetlinkManager() = default;

    NetlinkManager::~NetlinkManager()
    {
        cleanup();
    }

    int NetlinkManager::initialize()
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

    void NetlinkManager::cleanup()
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

    NetlinkResponse NetlinkManager::check_interface_exists(const std::string_view interface_name) const
    {
        // Refresh cache to get latest state
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, false, false);
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        const bool exists = (link != nullptr);
        
        if (link)
        {
            rtnl_link_put(link);
        }
        
        return NetlinkResponse(0, exists, false);
    }

    NetlinkResponse NetlinkManager::check_interface_is_up(const std::string_view interface_name) const
    {
        // Refresh cache to get latest state
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, false, false);
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        const bool exists = (link != nullptr);
        bool is_up = false;
        
        if (link)
        {
            is_up = (rtnl_link_get_flags(link) & IFF_UP) != 0;
            rtnl_link_put(link);
        }
        
        return NetlinkResponse(0, exists, is_up);
    }

    NetlinkResponse NetlinkManager::set_interface_state_libnl(std::string_view interface_name, const bool up) const
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

    NetlinkResponse NetlinkManager::set_can_bitrate_libnl(std::string_view interface_name, const uint32_t bitrate) const
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

    NetlinkResponse NetlinkManager::process_request(const NetlinkRequest& request) const
    {
        switch (request.operation)
        {
            case RequestType::INTERFACE_EXISTS:
                return check_interface_exists(request.interface_name);
            
            case RequestType::INTERFACE_IS_UP:
                return check_interface_is_up(request.interface_name);
            
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
            
            case RequestType::SET_INTERFACE_STATE:
            default:
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
    }

} // namespace HyCAN