#include <HyCAN/Daemon/VCAN.hpp>
#include <format>
#include <cstring>
#include <cerrno>

#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>

using tl::unexpected, std::format;

namespace HyCAN
{
    tl::expected<void, Error> create_vcan_interface_if_not_exists(std::string_view interface_name) noexcept
    {
        if (if_nametoindex(interface_name.data()) != 0)
        {
            return {}; // already exist.
        }
        if (errno != ENODEV) // if_nametoindex failed for a reason other than "No such device"
        {
            return tl::make_unexpected(Error{
                ErrorCode::VCANCheckingError, format("Error checking interface '{}' before creation: {}",
                                                     interface_name,
                                                     strerror(errno))
            });
        }

        // Creating VCAN interface
        nl_sock* sock = nl_socket_alloc();
        if (!sock)
        {
            return unexpected(Error{ErrorCode::NlSocketAllocError, "Error: Cannot alloc nl_socket."});
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            return unexpected(Error{ErrorCode::NlConnectError, "Error: Cannot connect to nl_socket."});
        }

        rtnl_link* link = rtnl_link_alloc();
        if (!link)
        {
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{ErrorCode::RtnlLinkAllocError, "Error: Cannot alloc rtnl_link."});
        }

        rtnl_link_set_name(link, std::string(interface_name).c_str());
        rtnl_link_set_type(link, "vcan");

        const int result = rtnl_link_add(sock, link, NLM_F_CREATE);

        rtnl_link_put(link);
        nl_close(sock);
        nl_socket_free(sock);

        if (result < 0)
        {
            return unexpected(Error{
                ErrorCode::VCANCreationError,
                format("Failed to create VCAN interface '{}': {}", interface_name, strerror(-result))
            });
        }

        return {};
    }
} // namespace HyCAN
