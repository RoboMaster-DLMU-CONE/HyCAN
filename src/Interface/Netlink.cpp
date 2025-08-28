#include "HyCAN/Interface/Netlink.hpp"
#include "HyCAN/Interface/VCAN.hpp"

#include <stdexcept>
#include <format>
#include <cstdlib>
#include <iostream>

#include <net/if.h>


using tl::unexpected, std::string_view;

namespace HyCAN
{
    Netlink::Netlink(const string_view interface_name) : interface_name(interface_name)
    {
        // TODO: only create vcan if user want it. at runtime.
        // Don't throw if VCAN creation fails - the interface might be a real CAN interface
        (void)create_vcan_interface_if_not_exists(interface_name).map_error([](const auto& e)
        {
            // Just log the error instead of throwing - this allows testing without VCAN modules
            std::cerr << "Warning: " << e.message << " (continuing anyway)" << std::endl;
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

    //TODO: remove useless template
    template <bool state>
    tl::expected<void, Error> Netlink::set_sock() noexcept
    {
        // Check if interface exists first
        if (if_nametoindex(interface_name.data()) == 0)
        {
            return unexpected(Error{
                ErrorCode::NetlinkInterfaceNotFound, format("Interface {} not found", interface_name)
            });
        }

        // Determine the action and construct the command
        std::string command;
        if constexpr (state == 0)
        {
            // Bring interface down
            command = format("ip link set {} down", interface_name);
        }
        else
        {
            // For CAN interfaces, set bitrate before bringing up
            if (interface_name.starts_with("can"))
            {
                // Set bitrate first
                std::string bitrate_cmd = format("ip link set {} type can bitrate 1000000", interface_name);
                if (const int bitrate_result = std::system(bitrate_cmd.c_str()); bitrate_result != 0)
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        format("Failed to set bitrate for interface {}: system() returned {}", interface_name, bitrate_result)
                    });
                }
            }
            // Bring interface up
            command = format("ip link set {} up", interface_name);
        }

        // Execute the command
        if (const int result = std::system(command.c_str()); result != 0)
        {
            if constexpr (state == 0)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringDownError,
                    format("Failed to bring down interface {}: system() returned {}", interface_name, result)
                });
            }
            else
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    format("Failed to bring up interface {}: system() returned {}", interface_name, result)
                });
            }
        }
        
        return {};
    }

    template tl::expected<void, Error> Netlink::set_sock<true>() noexcept;
    template tl::expected<void, Error> Netlink::set_sock<false>() noexcept;
}
