#include "HyCAN/Interface/Netlink.hpp"
#include "HyCAN/Interface/VCAN.hpp"

#include <stdexcept>
#include <format>

#include <net/if.h>

#include <netlink/netlink.h>
#include <netlink/route/link.h>


using tl::unexpected, std::string_view;

namespace HyCAN
{
    Netlink::Netlink(const string_view interface_name) : interface_name(interface_name)
    {
        (void)create_vcan_interface_if_not_exists(interface_name).map_error([](const auto& e)
        {
            throw std::runtime_error(e.message);
        });
    }

    tl::expected<void, Error> Netlink::up() noexcept
    {
        return set_sock<true>();
    }

    tl::expected<void, Error> Netlink::down() noexcept
    {
        return set_sock<false>();
    }

    template <bool state>
    tl::expected<void, Error> Netlink::set_sock() noexcept
    {
        nl_sock* sock = nl_socket_alloc();
        if (!sock)
        {
            return unexpected(Error{
                ErrorCode::NetlinkSocketNotInitialized, format("Netlink socket not initialized for {}", interface_name)
            });
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, format("Failed to connect to netlink socket for {}", interface_name)
            });
        }
        // Setting up / down Netlink

        const int ifindex = static_cast<int>(if_nametoindex(interface_name.data()));
        if (ifindex == 0)
        {
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkInterfaceNotFound, format("Interface {} not found", interface_name)
            });
        }
        rtnl_link* link = rtnl_link_alloc();
        rtnl_link* change = rtnl_link_alloc();
        if (!link || !change)
        {
            if (link) rtnl_link_put(link);
            if (change) rtnl_link_put(change);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::RtnlLinkAllocError, format("Failed to allocate rtnl_link for {}", interface_name)
            });
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
            rtnl_link_put(link);
            rtnl_link_put(change);
            nl_close(sock);
            nl_socket_free(sock);
            if constexpr (state == 0)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringDownError,
                    format("Failed to bring down interface {}: {}", interface_name, nl_geterror(err))
                });
            }
            else
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    format("Failed to bring up interface {}: {}", interface_name, nl_geterror(err))
                });
            }
        }
        rtnl_link_put(link);
        rtnl_link_put(change);
        nl_close(sock);
        nl_socket_free(sock);
        return {};
    }

    template tl::expected<void, Error> Netlink::set_sock<true>() noexcept;
    template tl::expected<void, Error> Netlink::set_sock<false>() noexcept;
}
