#ifndef HYCAN_DAEMON_HPP
#define HYCAN_DAEMON_HPP

#include <string_view>
#include <cstdint>
#include <iostream>
#include <string>
#include <net/if.h>
#include <cstring>
#include <sys/types.h>

namespace HyCAN
{
    enum class RequestType : uint8_t
    {
        SET_INTERFACE_STATE = 0,
        CHECK_INTERFACE_STATE = 1,
        VALIDATE_CAN_HARDWARE = 2,
        CREATE_VCAN_INTERFACE = 3,
        CLIENT_REGISTER = 4,
        INTERFACE_EXISTS = 5,
        INTERFACE_IS_UP = 6
    };


    /**
     * @brief Request structure for client registration
     */
    struct ClientRegisterRequest
    {
        pid_t client_pid;

        explicit ClientRegisterRequest(pid_t pid = 0) : client_pid(pid)
        {
        }
    };

    /**
     * @brief Response structure for client registration
     */
    struct ClientRegisterResponse
    {
        int result;
        char client_channel_name[64]{};

        explicit ClientRegisterResponse(int res = 0, const std::string_view channel_name = "")
            : result(res)
        {
            if (!channel_name.empty())
            {
                std::strncpy(client_channel_name, channel_name.data(), sizeof client_channel_name - 1);
                client_channel_name[sizeof client_channel_name - 1] = 0;
            }
        }
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
        char interface_name[IFNAMSIZ]{}; // 使用固定大小的字符数组

        NetlinkRequest() = default;

        explicit NetlinkRequest(const std::string_view name, const bool state, const bool bitrate_flag = false,
                                const uint32_t rate = 1000000)
            : up(state), set_bitrate(bitrate_flag), bitrate(rate)
        {
            std::strncpy(interface_name, name.data(), sizeof interface_name - 1);
            interface_name[sizeof interface_name - 1] = 0;
        }

        // Constructor for query operations
        explicit NetlinkRequest(RequestType op, const std::string_view name)
            : operation(op)
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
        bool exists{false}; // For interface exists query
        bool is_up{false}; // For interface up status query
        char error_message[256]{};

        explicit NetlinkResponse(const int res = 0, const std::string_view msg = "") : result(res)
        {
            if (!msg.empty())
            {
                std::strncpy(error_message, msg.data(), sizeof error_message - 1);
                error_message[sizeof error_message - 1] = 0;
            }
        }

        // Constructor for query responses
        explicit NetlinkResponse(const int res, bool interface_exists, bool interface_up)
            : result(res), exists(interface_exists), is_up(interface_up)
        {
        }
    };
} // namespace HyCAN

#endif
