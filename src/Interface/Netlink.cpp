#include "HyCAN/Interface/Netlink.hpp"
#include "HyCAN/Daemon/Daemon.hpp"

#include <stdexcept>
#include <format>
#include <cstring>
#include <net/if.h>

#include <libipc/ipc.h>

using tl::unexpected, std::string_view;

namespace HyCAN
{
    class NetlinkClient
    {
        ipc::channel channel_{"HyCAN_Daemon", ipc::sender};

        tl::expected<void, Error> send_daemon_request(std::string_view interface_name, const bool state)
        {
            try
            {
                // 准备请求
                const bool is_can_interface = interface_name.starts_with("can");
                const bool needs_vcan_creation = interface_name.starts_with("vcan");
                NetlinkRequest request{interface_name, state, is_can_interface && state, 1000000, needs_vcan_creation};

                // 发送请求
                const auto request_data = ipc::buff_t{
                    reinterpret_cast<ipc::byte_t*>(&request),
                    sizeof(request)
                };

                if (!channel_.send(request_data))
                {
                    return fallback_system_call(interface_name, state);
                }

                // 接收响应，超时 5 秒
                auto response_data = channel_.recv(5000);
                if (response_data.empty() || response_data.size() != sizeof(NetlinkResponse))
                {
                    return fallback_system_call(interface_name, state);
                }

                const auto* response = static_cast<const NetlinkResponse*>(response_data.data());

                if (response->result != 0)
                {
                    return unexpected(Error{
                        state ? ErrorCode::NetlinkBringUpError : ErrorCode::NetlinkBringDownError,
                        std::format("Daemon failed to {} interface {}: {}",
                                    state ? "bring up" : "bring down", interface_name, response->error_message)
                    });
                }

                return {};
            }
            catch (const std::exception&)
            {
                return fallback_system_call(interface_name, state);
            }
        }

        static tl::expected<void, Error> fallback_system_call(std::string_view interface_name, const bool state)
        {
            std::string command;
            if (state)
            {
                if (interface_name.starts_with("can"))
                {
                    const std::string bitrate_cmd = std::format("sudo ip link set {} type can bitrate 1000000",
                                                                interface_name);
                    if (const int bitrate_result = std::system(bitrate_cmd.c_str()); bitrate_result != 0)
                    {
                        return unexpected(Error{
                            ErrorCode::NetlinkBringUpError,
                            std::format("Failed to set bitrate for interface {}: system() returned {}",
                                        interface_name, bitrate_result)
                        });
                    }
                }
                command = std::format("sudo ip link set {} up", interface_name);
            }
            else
            {
                command = std::format("sudo ip link set {} down", interface_name);
            }

            if (const int result = std::system(command.c_str()); result != 0)
            {
                return unexpected(Error{
                    state ? ErrorCode::NetlinkBringUpError : ErrorCode::NetlinkBringDownError,
                    std::format("Failed to {} interface {}: system() returned {}",
                                state ? "bring up" : "bring down", interface_name, result)
                });
            }
            return {};
        }

    public:
        tl::expected<void, Error> set_interface_state(const std::string_view interface_name, const bool up)
        {
            return send_daemon_request(interface_name, up);
        }
    };

    Netlink::Netlink(const string_view interface_name) : interface_name(interface_name)
    {
    }

    tl::expected<void, Error> Netlink::up() noexcept
    {
        NetlinkClient client;
        return client.set_interface_state(interface_name, true);
    }

    tl::expected<void, Error> Netlink::down() noexcept
    {
        NetlinkClient client;
        return client.set_interface_state(interface_name, false);
    }
} // namespace HyCAN
