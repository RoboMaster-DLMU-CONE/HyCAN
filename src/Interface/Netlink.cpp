#include "HyCAN/Interface/Netlink.hpp"
#include "HyCAN/Daemon/Daemon.hpp"

#include <stdexcept>
#include <format>
#include <cstring>
#include <cerrno>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

        tl::expected<bool, Error> check_interface_state(std::string_view interface_name)
        {
            try
            {
                // 准备检查状态请求
                NetlinkRequest request{interface_name, RequestType::CHECK_INTERFACE_STATE};

                // 发送请求
                const auto request_data = ipc::buff_t{
                    reinterpret_cast<ipc::byte_t*>(&request),
                    sizeof(request)
                };

                if (!channel_.send(request_data))
                {
                    return fallback_check_state(interface_name);
                }

                // 接收响应，超时 5 秒
                auto response_data = channel_.recv(5000);
                if (response_data.empty() || response_data.size() != sizeof(NetlinkResponse))
                {
                    return fallback_check_state(interface_name);
                }

                const auto* response = static_cast<const NetlinkResponse*>(response_data.data());

                if (response->result != 0)
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError, // Reuse existing error code
                        std::format("Daemon failed to check interface {} state: {}",
                                    interface_name, response->error_message)
                    });
                }

                return response->interface_up;
            }
            catch (const std::exception&)
            {
                return fallback_check_state(interface_name);
            }
        }

        static tl::expected<bool, Error> fallback_check_state(std::string_view interface_name)
        {
            // Fallback using system commands similar to the test helper
            const int fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd < 0)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to create socket for checking interface {} state", interface_name)
                });
            }

            ifreq ifr{};
            const auto name_len = std::min(interface_name.size(), static_cast<size_t>(IFNAMSIZ - 1));
            std::memcpy(ifr.ifr_name, interface_name.data(), name_len);
            ifr.ifr_name[name_len] = '\0';

            if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
            {
                close(fd);
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to get interface {} flags: {}", interface_name, strerror(errno))
                });
            }

            close(fd);
            return (ifr.ifr_flags & IFF_UP) != 0;
        }

    public:
        tl::expected<void, Error> set_interface_state(const std::string_view interface_name, const bool up)
        {
            return send_daemon_request(interface_name, up);
        }

        tl::expected<bool, Error> get_interface_state(const std::string_view interface_name)
        {
            return check_interface_state(interface_name);
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

    tl::expected<bool, Error> Netlink::is_up() noexcept
    {
        NetlinkClient client;
        return client.get_interface_state(interface_name);
    }
} // namespace HyCAN
