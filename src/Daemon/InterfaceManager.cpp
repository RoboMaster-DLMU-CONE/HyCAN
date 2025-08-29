#include "HyCAN/Daemon/InterfaceManager.hpp"
#include "HyCAN/Daemon/Daemon.hpp"

#include <format>
#include <net/if.h>
#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>

namespace HyCAN
{
    InterfaceManager::InterfaceManager(nl_sock* socket, nl_cache* cache)
        : nl_socket_(socket), link_cache_(cache)
    {
    }

    NetlinkResponse InterfaceManager::set_interface_state(std::string_view interface_name, const bool up) const
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

    NetlinkResponse InterfaceManager::check_interface_state(std::string_view interface_name) const
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
}