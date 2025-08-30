#include <HyCAN/Interface/Netlink.hpp>
#include <HyCAN/Daemon/Message.hpp>
#include <HyCAN/Daemon/UnixSocket/UnixSocket.hpp>

#include <format>
#include <cstring>
#include <net/if.h>
#include <unistd.h>
#include <memory>

using tl::unexpected, std::string_view;

namespace HyCAN
{
    /**
     * @brief Internal client for communicating with HyCAN daemon
     */
    class NetlinkClient
    {
    private:
        std::string client_channel_name_;
        bool registered_{false};

    public:
        tl::expected<void, Error> ensure_registered()
        {
            if (registered_)
            {
                return {};
            }

            try
            {
                // Create socket for registration
                const auto registration_socket = std::make_unique<UnixSocket>("daemon", UnixSocket::CLIENT);
                if (!registration_socket->initialize())
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to connect to daemon for registration"
                    });
                }

                // Send registration request
                const ClientRegisterRequest register_request(getpid());
                if (registration_socket->send(&register_request, sizeof(register_request)) < 0)
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to send registration request to daemon"
                    });
                }

                // Receive registration response
                ClientRegisterResponse response;
                const ssize_t bytes_received = registration_socket->recv(&response, sizeof(response), 5000);
                
                if (bytes_received != sizeof(ClientRegisterResponse))
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to receive registration response from daemon"
                    });
                }

                if (response.result != 0)
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Daemon failed to register client"
                    });
                }

                // Set up client channel name
                client_channel_name_ = response.client_channel_name;
                registered_ = true;
                return {};
            }
            catch (const std::exception& e)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Exception during registration: {}", e.what())
                });
            }
        }

        tl::expected<NetlinkResponse, Error> send_request(const NetlinkRequest& request)
        {
            auto reg_result = ensure_registered();
            if (!reg_result)
            {
                return unexpected(reg_result.error());
            }

            try
            {
                // Create client socket connection
                const auto client_socket = std::make_unique<UnixSocket>(client_channel_name_, UnixSocket::CLIENT);
                if (!client_socket->initialize())
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to connect to daemon client channel"
                    });
                }

                // Send request
                if (client_socket->send(&request, sizeof(request)) < 0)
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to send request to daemon"
                    });
                }

                // Receive response
                NetlinkResponse response;
                const ssize_t bytes_received = client_socket->recv(&response, sizeof(response), 5000);
                
                if (bytes_received != sizeof(NetlinkResponse))
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to receive response from daemon"
                    });
                }

                return response;
            }
            catch (const std::exception& e)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Exception during request: {}", e.what())
                });
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
            const bool is_can_interface = interface_name.starts_with("can");
            const bool needs_vcan_creation = interface_name.starts_with("vcan");
            const NetlinkRequest request{interface_name, up, is_can_interface && up, 1000000, needs_vcan_creation};

            auto response_result = send_request(request);
            if (!response_result)
            {
                // Fallback to system call
                return fallback_system_call(interface_name, up);
            }

            const auto& response = response_result.value();
            if (response.result != 0)
            {
                return unexpected(Error{
                    up ? ErrorCode::NetlinkBringUpError : ErrorCode::NetlinkBringDownError,
                    std::format("Daemon failed to {} interface {}: {}",
                                up ? "bring up" : "bring down", interface_name, response.error_message)
                });
            }

            return {};
        }

        tl::expected<bool, Error> interface_exists(const std::string_view interface_name)
        {
            NetlinkRequest request{RequestType::INTERFACE_EXISTS, interface_name};

            auto response_result = send_request(request);
            if (!response_result)
            {
                return unexpected(response_result.error());
            }

            const auto& response = response_result.value();
            if (response.result != 0)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to check if interface {} exists: {}", interface_name, response.error_message)
                });
            }

            return response.exists;
        }

        tl::expected<bool, Error> interface_is_up(const std::string_view interface_name)
        {
            NetlinkRequest request{RequestType::INTERFACE_IS_UP, interface_name};

            auto response_result = send_request(request);
            if (!response_result)
            {
                return unexpected(response_result.error());
            }

            const auto& response = response_result.value();
            if (response.result != 0)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to check if interface {} is up: {}", interface_name, response.error_message)
                });
            }

            return response.is_up;
        }

        tl::expected<void, Error> create_vcan_interface(const std::string_view interface_name)
        {
            const NetlinkRequest request{RequestType::CREATE_VCAN_INTERFACE, interface_name};

            auto response_result = send_request(request);
            if (!response_result)
            {
                return unexpected(response_result.error());
            }

            const auto& response = response_result.value();
            if (response.result != 0)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("Failed to create VCAN interface {}: {}", interface_name, response.error_message)
                });
            }

            return {};
        }
    };

    // Netlink singleton implementation
    Netlink::Netlink() : main_channel_name_("HyCAN_Daemon")
    {
    }

    Netlink::~Netlink() 
    {
        // Explicitly reset the client before destruction
        client_.reset();
    }

    Netlink& Netlink::instance()
    {
        static Netlink instance;
        return instance;
    }

    tl::expected<void, Error> Netlink::ensure_initialized()
    {
        if (initialized_)
        {
            return {};
        }

        try
        {
            client_ = std::make_unique<NetlinkClient>();
            initialized_ = true;
            return {};
        }
        catch (const std::exception& e)
        {
            return unexpected(Error{
                ErrorCode::NetlinkBringUpError,
                std::format("Failed to initialize Netlink: {}", e.what())
            });
        }
    }

    tl::expected<void, Error> Netlink::set(const std::string_view interface_name, const bool up)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->set_interface_state(interface_name, up);
    }

    tl::expected<bool, Error> Netlink::exists(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->interface_exists(interface_name);
    }

    tl::expected<bool, Error> Netlink::is_up(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->interface_is_up(interface_name);
    }

    tl::expected<void, Error> Netlink::create_vcan(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->create_vcan_interface(interface_name);
    }
} // namespace HyCAN