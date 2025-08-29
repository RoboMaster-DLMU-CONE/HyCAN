#include "HyCAN/Daemon/RequestProcessor.hpp"
#include "HyCAN/Daemon/Daemon.hpp"
#include "HyCAN/Daemon/InterfaceManager.hpp"
#include "HyCAN/Daemon/CANManager.hpp"
#include "HyCAN/Daemon/VCAN.hpp"

#include <format>
#include <iostream>

namespace HyCAN
{
    RequestProcessor::RequestProcessor(const InterfaceManager& interface_mgr, const CANManager& can_mgr)
        : interface_manager_(interface_mgr), can_manager_(can_mgr)
    {
    }

    NetlinkResponse RequestProcessor::process_request(const NetlinkRequest& request) const
    {
        switch (request.operation)
        {
            case RequestType::SET_INTERFACE_STATE:
            {
                // Create VCAN interface if needed
                if (request.create_vcan_if_needed)
                {
                    auto vcan_result = create_vcan_interface_if_not_exists(request.interface_name);
                    if (!vcan_result)
                    {
                        return NetlinkResponse(-1, std::format("Failed to create VCAN interface {}: {}",
                                                               request.interface_name, vcan_result.error().message));
                    }
                }

                // 如果需要设置比特率（仅对 CAN 接口）
                if (request.up && request.set_bitrate)
                {
                    const auto bitrate_result = can_manager_.set_can_bitrate(request.interface_name, request.bitrate);
                    if (bitrate_result.result != 0)
                    {
                        return bitrate_result;
                    }
                }

                // 设置接口状态
                return interface_manager_.set_interface_state(request.interface_name, request.up);
            }
            case RequestType::CHECK_INTERFACE_STATE:
                return interface_manager_.check_interface_state(request.interface_name);
            case RequestType::VALIDATE_CAN_HARDWARE:
                return can_manager_.validate_can_hardware(request.interface_name);
            case RequestType::CREATE_VCAN_INTERFACE:
            {
                std::cout << "Processing CREATE_VCAN_INTERFACE for " << request.interface_name << std::endl;
                auto vcan_result = create_vcan_interface_if_not_exists(request.interface_name);
                if (!vcan_result)
                {
                    std::cout << "VCAN creation failed: " << vcan_result.error().message << std::endl;
                    return NetlinkResponse(-1, std::format("Failed to create VCAN interface {}: {}",
                                                           request.interface_name, vcan_result.error().message));
                }
                std::cout << "VCAN creation successful for " << request.interface_name << std::endl;
                return NetlinkResponse(0, "VCAN interface created successfully");
            }
            default:
                return NetlinkResponse(-1, "Unknown request operation");
        }
    }
}