#include "HyCAN/Interface/Netlink.hpp"
#include "HyCAN/Daemon/Message.hpp"

#include <stdexcept>
#include <format>
#include <cstring>
#include <net/if.h>
#include <unistd.h>
#include <memory>

#include <libipc/ipc.h>

using tl::unexpected, std::string_view;

namespace HyCAN
{
    /**
     * @brief Internal client for communicating with HyCAN daemon
     */
    class NetlinkClient
    {
    private:
        std::string main_channel_name_;
        std::string client_channel_name_;
        std::unique_ptr<ipc::channel> main_channel_;
        std::unique_ptr<ipc::channel> client_channel_;
        bool registered_{false};

    public:
        ~NetlinkClient()
        {
            // Explicit cleanup to ensure proper order
            client_channel_.reset();
            main_channel_.reset();
        }

        tl::expected<void, Error> ensure_registered()
        {
            if (registered_)
            {
                return {};
            }

            try
            {
                // Create main channel for registration
                main_channel_ = std::make_unique<ipc::channel>("HyCAN_Daemon", ipc::sender | ipc::receiver);

                // Send registration request
                ClientRegisterRequest register_request(getpid());
                auto request_data = ipc::buff_t{
                    reinterpret_cast<ipc::byte_t*>(&register_request),
                    sizeof(register_request)
                };

                if (!main_channel_->send(request_data))
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to send registration request to daemon"
                    });
                }

                // Receive registration response
                auto response_data = main_channel_->recv(5000); // 5 second timeout
                if (response_data.empty() || response_data.size() != sizeof(ClientRegisterResponse))
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to receive registration response from daemon"
                    });
                }

                const auto* response = static_cast<const ClientRegisterResponse*>(response_data.data());
                if (response->result != 0)
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Daemon failed to register client"
                    });
                }

                // Set up client channel
                client_channel_name_ = response->client_channel_name;
                client_channel_ = std::make_unique<ipc::channel>(client_channel_name_.c_str(), ipc::sender | ipc::receiver);

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
                // Send request
                auto request_data = ipc::buff_t{
                    reinterpret_cast<ipc::byte_t*>(const_cast<NetlinkRequest*>(&request)),
                    sizeof(request)
                };

                if (!client_channel_->send(request_data))
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to send request to daemon"
                    });
                }

                // Receive response
                auto response_data = client_channel_->recv(5000); // 5 second timeout
                if (response_data.empty() || response_data.size() != sizeof(NetlinkResponse))
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkBringUpError,
                        "Failed to receive response from daemon"
                    });
                }

                const auto* response = static_cast<const NetlinkResponse*>(response_data.data());
                return *response;
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
            NetlinkRequest request{interface_name, up, is_can_interface && up, 1000000, needs_vcan_creation};

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
            NetlinkRequest request{RequestType::CREATE_VCAN_INTERFACE, interface_name};

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

    // NetlinkManager singleton implementation
    NetlinkManager::NetlinkManager() : main_channel_name_("HyCAN_Daemon")
    {
    }

    NetlinkManager::~NetlinkManager() 
    {
        // Explicitly reset the client before destruction
        client_.reset();
    }

    NetlinkManager& NetlinkManager::instance()
    {
        static NetlinkManager instance;
        return instance;
    }

    tl::expected<void, Error> NetlinkManager::ensure_initialized()
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
                std::format("Failed to initialize NetlinkManager: {}", e.what())
            });
        }
    }

    tl::expected<void, Error> NetlinkManager::set(std::string_view interface_name, bool up)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->set_interface_state(interface_name, up);
    }

    tl::expected<bool, Error> NetlinkManager::exists(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->interface_exists(interface_name);
    }

    tl::expected<bool, Error> NetlinkManager::is_up(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->interface_is_up(interface_name);
    }

    tl::expected<void, Error> NetlinkManager::create_vcan(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->create_vcan_interface(interface_name);
    }

    // Legacy Netlink class implementation for backward compatibility
    Netlink::Netlink(const string_view interface_name) : interface_name_(interface_name)
    {
    }

    tl::expected<void, Error> Netlink::up() noexcept
    {
        return NetlinkManager::instance().set(interface_name_, true);
    }

    tl::expected<void, Error> Netlink::down() noexcept
    {
        return NetlinkManager::instance().set(interface_name_, false);
    }
} // namespace HyCAN