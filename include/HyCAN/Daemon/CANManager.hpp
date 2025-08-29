#ifndef HYCAN_CAN_MANAGER_HPP
#define HYCAN_CAN_MANAGER_HPP

#include <string_view>
#include <cstdint>

struct nl_sock;
struct nl_cache;

namespace HyCAN
{
    struct NetlinkResponse;

    /**
     * @brief Manages CAN-specific operations (bitrate, hardware validation)
     */
    class CANManager
    {
        nl_sock* nl_socket_;
        nl_cache* link_cache_;

    public:
        CANManager(nl_sock* socket, nl_cache* cache);

        NetlinkResponse set_can_bitrate(std::string_view interface_name, uint32_t bitrate) const;
        NetlinkResponse validate_can_hardware(std::string_view interface_name) const;
    };
}

#endif