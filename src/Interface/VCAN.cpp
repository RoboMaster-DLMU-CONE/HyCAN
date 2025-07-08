#include "HyCAN/Interface/VCAN.hpp"
#include <format>

#include <net/if.h>

#include <netlink/netlink.h>
#include <netlink/route/link.h>

using tl::unexpected, std::format;

namespace HyCAN
{
    Result create_vcan_interface_if_not_exists(const std::string_view interface_name) noexcept
    {
        if (if_nametoindex(interface_name.data()) != 0)
        {
            return {}; // already exist.
        }
        if (errno != ENODEV) // if_nametoindex failed for a reason other than "No such device"
        {
            return unexpected(format("Error checking interface '{}' before creation: {}", interface_name,
                                     strerror(errno)));
        }

        // Creating VCAN for Interface.

        nl_sock* sock;
        rtnl_link* link;
        if (sock = nl_socket_alloc(); !sock)
        {
            return unexpected("Error: Cannot alloc nl_socket.");
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            return unexpected("Error: Cannot connected to nl_socket.");
        }


        if (link = rtnl_link_alloc(); !link)
        {
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected("Error: Cannot alloc rtnl_link.");
        }

        rtnl_link_set_name(link, interface_name.data());
        rtnl_link_set_type(link, "vcan");

        if (const int err = rtnl_link_add(sock, link, NLM_F_CREATE); err < 0)
        {
            rtnl_link_put(link);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(format("Error: Cannot create interface: {}.", nl_geterror(err)));
        }
        rtnl_link_put(link);
        nl_close(sock);
        nl_socket_free(sock);
        return {};
    }
}
