#include "HyCAN/Interface/CANInterface.hpp"
#include "HyCAN/Daemon/Daemon.hpp"

#include <stdexcept>
#include <format>
#include <libipc/ipc.h>

using tl::unexpected;

namespace HyCAN
{
    CANInterface::CANInterface(const std::string& interface_name) 
        : Interface(interface_name)
    {
        // Validate that the CAN hardware interface exists during construction
        auto validation_result = validate_can_hardware();
        if (!validation_result)
        {
            throw std::runtime_error(
                std::format("CAN interface '{}' validation failed: {}", 
                           interface_name, validation_result.error().message)
            );
        }
    }

    tl::expected<void, Error> CANInterface::up()
    {
        // For CAN interfaces, we need to validate hardware first, then bring up
        return validate_can_hardware()
            .and_then([&] { return netlink.up(); })
            .and_then([&] { return reaper.start(); });
    }

    tl::expected<void, Error> CANInterface::validate_can_hardware()
    {
        try
        {
            ipc::channel channel{"HyCAN_Daemon", ipc::sender};
            
            // Prepare validation request
            NetlinkRequest request{interface_name, RequestType::VALIDATE_CAN_HARDWARE};

            // Send request
            const auto request_data = ipc::buff_t{
                reinterpret_cast<ipc::byte_t*>(&request),
                sizeof(request)
            };

            if (!channel.send(request_data))
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to send CAN hardware validation request for interface {}", interface_name)
                });
            }

            // Receive response, timeout 5 seconds
            auto response_data = channel.recv(5000);
            if (response_data.empty() || response_data.size() != sizeof(NetlinkResponse))
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to receive CAN hardware validation response for interface {}", interface_name)
                });
            }

            const auto* response = static_cast<const NetlinkResponse*>(response_data.data());

            if (response->result != 0 || !response->hardware_exists)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("CAN hardware interface {} not found or not accessible: {}",
                               interface_name, response->error_message)
                });
            }

            return {};
        }
        catch (const std::exception& e)
        {
            return unexpected(Error{
                ErrorCode::NetlinkBringUpError,
                std::format("Exception during CAN hardware validation for interface {}: {}", interface_name, e.what())
            });
        }
    }
}