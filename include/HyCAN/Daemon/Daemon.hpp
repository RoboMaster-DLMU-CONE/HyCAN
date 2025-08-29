#ifndef HYCAN_DAEMON_HPP
#define HYCAN_DAEMON_HPP

#include <string_view>
#include <cstdint>
#include <net/if.h>

namespace HyCAN
{
    /**
     * @brief Request operation types for daemon communication
     */
    enum class RequestType : uint8_t
    {
        SET_INTERFACE_STATE = 0,  // Bring interface up/down
        CHECK_INTERFACE_STATE = 1, // Check if interface is up
        VALIDATE_CAN_HARDWARE = 2, // Check if CAN hardware exists
        CREATE_VCAN_INTERFACE = 3  // Create VCAN interface
    };

    /**
     * @brief Request structure for daemon communication
     */
    struct NetlinkRequest
    {
        RequestType operation{RequestType::SET_INTERFACE_STATE};
        char interface_name[IFNAMSIZ]{};
        bool up{};
        bool set_bitrate{};
        uint32_t bitrate{};
        bool create_vcan_if_needed{}; // New field for VCAN creation

        NetlinkRequest() = default;

        explicit NetlinkRequest(const std::string_view name, const bool state, const bool bitrate_flag = false,
                                const uint32_t rate = 1000000, const bool create_vcan = false)
            : operation(RequestType::SET_INTERFACE_STATE), up(state), set_bitrate(bitrate_flag), bitrate(rate), create_vcan_if_needed(create_vcan)
        {
            const auto copy_size = std::min(name.size(), static_cast<size_t>(IFNAMSIZ - 1));
            name.copy(interface_name, copy_size);
            interface_name[copy_size] = '\0';
        }

        // Constructor for check interface state
        explicit NetlinkRequest(const std::string_view name, RequestType op)
            : operation(op)
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
        bool interface_up{false}; // For CHECK_INTERFACE_STATE requests
        bool hardware_exists{false}; // For VALIDATE_CAN_HARDWARE requests
        char error_message[256]{};

        explicit NetlinkResponse(int res = 0, std::string_view msg = "") : result(res)
        {
            const auto copy_size = std::min(msg.size(), sizeof(error_message) - 1);
            msg.copy(error_message, copy_size);
            error_message[copy_size] = '\0';
        }

        // Constructor for interface state check
        explicit NetlinkResponse(int res, bool up, std::string_view msg = "")
            : result(res), interface_up(up)
        {
            const auto copy_size = std::min(msg.size(), sizeof(error_message) - 1);
            msg.copy(error_message, copy_size);
            error_message[copy_size] = '\0';
        }

        // Constructor for hardware validation
        explicit NetlinkResponse(int res, bool exists, bool /* tag */, std::string_view msg = "")
            : result(res), hardware_exists(exists)
        {
            const auto copy_size = std::min(msg.size(), sizeof(error_message) - 1);
            msg.copy(error_message, copy_size);
            error_message[copy_size] = '\0';
        }
    };
} // namespace HyCAN

#endif
