#include "HyCAN/Daemon/CANManager.hpp"
#include "HyCAN/Daemon/Daemon.hpp"

#include <format>
#include <net/if.h>
#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>

namespace HyCAN
{
    CANManager::CANManager(nl_sock* socket, nl_cache* cache)
        : nl_socket_(socket), link_cache_(cache)
    {
    }

    NetlinkResponse CANManager::set_can_bitrate(std::string_view interface_name, const uint32_t bitrate) const
    {
        // 刷新缓存以获取最新状态
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, "Failed to refresh link cache");
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

    NetlinkResponse CANManager::validate_can_hardware(std::string_view interface_name) const
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
}