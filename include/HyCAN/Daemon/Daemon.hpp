#ifndef HYCAN_DAEMON_HPP
#define HYCAN_DAEMON_HPP

#include <string_view>
#include <cstdint>
#include <net/if.h>

namespace HyCAN
{
    /**
     * @brief Request structure for daemon communication
     */
    struct NetlinkRequest
    {
        char interface_name[IFNAMSIZ]{};
        bool up{};
        bool set_bitrate{};
        uint32_t bitrate{};
        bool create_vcan_if_needed{}; // New field for VCAN creation

        NetlinkRequest() = default;

        explicit NetlinkRequest(const std::string_view name, const bool state, const bool bitrate_flag = false,
                                const uint32_t rate = 1000000, const bool create_vcan = false)
            : up(state), set_bitrate(bitrate_flag), bitrate(rate), create_vcan_if_needed(create_vcan)
        {
            const auto copy_size = std::min(name.size(), static_cast<size_t>(IFNAMSIZ - 1));
            name.copy(interface_name, copy_size);
            interface_name[copy_size] = '\0';
        }
    };

    /**
     * @brief Response structure for daemon communication
     */
    struct NetlinkResponse
    {
        int result;
        char error_message[256]{};

        explicit NetlinkResponse(int res = 0, std::string_view msg = "") : result(res)
        {
            const auto copy_size = std::min(msg.size(), sizeof(error_message) - 1);
            msg.copy(error_message, copy_size);
            error_message[copy_size] = '\0';
        }
    };
} // namespace HyCAN

#endif
