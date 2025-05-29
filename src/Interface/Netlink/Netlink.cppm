module;
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <xtr/logger.hpp>

export module HyCAN.Interface.Netlink;
import HyCAN.Interface.Logger;
import :VCAN;

using std::format, std::string_view, std::string;
using xtr::logger, xtr::sink;

export namespace HyCAN
{
    class Netlink
    {
    public:
        explicit Netlink(string_view interface_name);
        Netlink() = delete;
        void up();
        void down();

    private:
        template <bool state>
        void set_sock();
        string_view interface_name;
        sink s;
    };

    Netlink::Netlink(string_view interface_name): interface_name(interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Netlink_{}", interface_name));
        create_vcan_interface_if_not_exists(interface_name, s);
    }

    void Netlink::up()
    {
        set_sock<true>();
        XTR_LOGL(info, s, "Interface {} is up.", interface_name);
    }

    void Netlink::down()
    {
        set_sock<false>();
        XTR_LOGL(info, s, "Interface {} is down.", interface_name);
    }

    template <bool state>
    void Netlink::set_sock()
    {
        nl_sock* sock = nl_socket_alloc();
        if (!sock)
        {
            XTR_LOGL(error, s, "Netlink socket not initialized for {}, cannot bring up.", interface_name);
        }
        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            XTR_LOGL(fatal, s, "Failed to connect to netlink socket");
        }
        XTR_LOGL(info, s, "Setting up Netlink for {}", interface_name);
        const int ifindex = static_cast<int>(if_nametoindex(interface_name.data()));
        if (ifindex == 0)
        {
            XTR_LOGL(fatal, s, "Interface {} not found", interface_name);
        }
        rtnl_link* link = rtnl_link_alloc();
        rtnl_link* change = rtnl_link_alloc();
        if (!link || !change)
        {
            nl_close(sock);
            nl_socket_free(sock);
            XTR_LOGL(fatal, s, "Failed to allocate rtnl_link for {}", interface_name);
        }

        rtnl_link_set_ifindex(link, ifindex);
        if constexpr (state == 0)
        {
            rtnl_link_unset_flags(change, IFF_UP);
        }
        else
        {
            rtnl_link_set_flags(change, IFF_UP);
        }


        if (const int err = rtnl_link_change(sock, link, change, 0); err < 0)
        {
            if constexpr (state == 0)
            {
                XTR_LOGL(fatal, s, "Failed to bring up interface {}: {}", interface_name, nl_geterror(err));
            }
            else
            {
                XTR_LOGL(fatal, s, "Failed to bring down interface {}: {}", interface_name, nl_geterror(err));
            }
        }
        rtnl_link_put(link);
        rtnl_link_put(change);
        nl_close(sock);
        nl_socket_free(sock);
    }
}
