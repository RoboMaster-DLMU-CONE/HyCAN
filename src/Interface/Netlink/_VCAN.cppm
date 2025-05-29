module;
#include <cstring>
#include <expected>
#include <string>
#include <string_view>
#include <format>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#include <xtr/logger.hpp>
export module HyCAN.Interface.Netlink:VCAN;

using std::expected, std::unexpected, std::string, std::string_view, std::format, xtr::sink;

export namespace HyCAN
{
    void create_vcan_interface_if_not_exists(const string_view interface_name, sink& s)
    {
        XTR_LOGL(info, s, "Creating VCAN for {}", interface_name);
        if (if_nametoindex(interface_name.data()) != 0)
        {
            XTR_LOGL(info, s, "{} already exists.", interface_name);
            return; // Already exists
        }
        if (errno != ENODEV) // if_nametoindex failed for a reason other than "No such device"
        {
            XTR_LOGL(fatal, s, "Error checking interface '{}' before creation: {}", interface_name, strerror(errno));
        }

        nl_sock* sock;
        rtnl_link* link;
        if (sock = nl_socket_alloc(); !sock)
        {
            XTR_LOGL(fatal, s, "Error: Cannot alloc nl_socket.");
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            XTR_LOGL(fatal, s, "Error: Cannot connected to nl_socket.");
        }


        if (link = rtnl_link_alloc(); !link)
        {
            nl_close(sock);
            nl_socket_free(sock);
            XTR_LOGL(fatal, s, "Error: Cannot alloc rtnl_link.");
        }

        rtnl_link_set_name(link, interface_name.data());
        rtnl_link_set_type(link, "vcan");

        if (const int err = rtnl_link_add(sock, link, NLM_F_CREATE); err < 0)
        {
            rtnl_link_put(link);
            nl_close(sock);
            nl_socket_free(sock);
            XTR_LOGL(fatal, s, "Error: Cannot create interface: {}.", nl_geterror(err));
        }
        rtnl_link_put(link);
        nl_close(sock);
        nl_socket_free(sock);
    }
}
