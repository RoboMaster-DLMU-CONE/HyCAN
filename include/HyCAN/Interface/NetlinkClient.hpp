#ifndef NETLINK_CLIENT_HPP
#define NETLINK_CLIENT_HPP

#include <string>
#include <memory>
#include <tl/expected.hpp>
#include "HyCAN/Util/Error.hpp"
#include "HyCAN/Daemon/Message.hpp"

namespace HyCAN
{
    /**
     * @brief Internal client for communicating with HyCAN daemon
     */
    class NetlinkClient
    {
        std::string client_channel_name_;
        bool registered_{false};

    public:
        tl::expected<void, Error> ensure_registered();
        tl::expected<NetlinkResponse, Error> send_request(const NetlinkRequest& request);
        static tl::expected<void, Error> fallback_system_call(std::string_view interface_name, bool state, uint32_t bitrate = 1000000);

        // Interface operations
        tl::expected<void, Error> set_interface_state(std::string_view interface_name, bool up, uint32_t bitrate = 1000000);
        tl::expected<bool, Error> interface_exists(std::string_view interface_name);
        tl::expected<bool, Error> interface_is_up(std::string_view interface_name);
        tl::expected<void, Error> create_vcan_interface(std::string_view interface_name);
    };
}

#endif //NETLINK_CLIENT_HPP