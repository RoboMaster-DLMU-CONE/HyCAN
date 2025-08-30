#ifndef HYCAN_DAEMON_HPP
#define HYCAN_DAEMON_HPP

#include <string_view>
#include <cstdint>
#include <iostream>
#include <string>
#include <net/if.h>
#include <cstring>

namespace HyCAN
{
    enum class RequestType : uint8_t
    {
        SET_INTERFACE_STATE = 0,
        CHECK_INTERFACE_STATE = 1,
        VALIDATE_CAN_HARDWARE = 2,
        CREATE_VCAN_INTERFACE = 3
    };


    /**
     * @brief Request structure for daemon communication
     */
    struct NetlinkRequest
    {
        RequestType operation{RequestType::SET_INTERFACE_STATE};
        bool up{};
        bool set_bitrate{};
        uint32_t bitrate{};
        bool create_vcan_if_needed{};
        char interface_name[IFNAMSIZ]{}; // 使用固定大小的字符数组

        NetlinkRequest() = default;

        explicit NetlinkRequest(const std::string_view name, const bool state, const bool bitrate_flag = false,
                                const uint32_t rate = 1000000, const bool create_vcan = false)
            : up(state), set_bitrate(bitrate_flag), bitrate(rate), create_vcan_if_needed(create_vcan)
        {
            std::strncpy(interface_name, name.data(), sizeof interface_name - 1);
            interface_name[sizeof interface_name - 1] = 0;
        }
    };

    /**
     * @brief Response structure for daemon communication
     */
    struct NetlinkResponse
    {
        int result;
        char error_message[256]{};

        explicit NetlinkResponse(const int res = 0, const std::string_view msg = "") : result(res)
        {
            if (!msg.empty())
            {
                std::strncpy(error_message, msg.data(), sizeof error_message - 1);
                error_message[sizeof error_message - 1] = 0;
            }
        }
    };
} // namespace HyCAN

#endif
