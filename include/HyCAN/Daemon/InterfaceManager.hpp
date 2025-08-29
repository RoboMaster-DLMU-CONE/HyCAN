#ifndef HYCAN_INTERFACE_MANAGER_HPP
#define HYCAN_INTERFACE_MANAGER_HPP

#include <string_view>

struct nl_sock;
struct nl_cache;

namespace HyCAN
{
    struct NetlinkResponse;

    /**
     * @brief Manages interface state operations (up/down/check state)
     */
    class InterfaceManager
    {
        nl_sock* nl_socket_;
        nl_cache* link_cache_;

    public:
        InterfaceManager(nl_sock* socket, nl_cache* cache);

        NetlinkResponse set_interface_state(std::string_view interface_name, bool up) const;
        NetlinkResponse check_interface_state(std::string_view interface_name) const;
    };
}

#endif