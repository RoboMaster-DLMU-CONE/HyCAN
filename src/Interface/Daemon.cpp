#include "HyCAN/Interface/Daemon.hpp"

#include <format>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <errno.h>

#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>

using tl::unexpected, std::string_view, std::format;

namespace HyCAN
{
    // Protocol constants
    static constexpr size_t MAX_INTERFACE_NAME_LEN = 256;
    static constexpr size_t MAX_ERROR_MESSAGE_LEN = 1024;

    Daemon::Daemon() = default;

    Daemon::~Daemon()
    {
        stop();
    }

    tl::expected<void, Error> Daemon::start(const std::string& socket_path) noexcept
    {
        socket_path_ = socket_path;
        
        // Create Unix domain socket
        server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ == -1)
        {
            return unexpected(Error{
                ErrorCode::NetlinkSocketNotInitialized,
                format("Failed to create daemon socket: {}", strerror(errno))
            });
        }

        // Remove existing socket file if it exists
        unlink(socket_path_.c_str());

        // Bind socket
        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            close(server_fd_);
            server_fd_ = -1;
            return unexpected(Error{
                ErrorCode::NetlinkConnectError,
                format("Failed to bind daemon socket: {}", strerror(errno))
            });
        }

        // Listen for connections
        if (listen(server_fd_, 5) == -1)
        {
            close(server_fd_);
            server_fd_ = -1;
            return unexpected(Error{
                ErrorCode::NetlinkConnectError,
                format("Failed to listen on daemon socket: {}", strerror(errno))
            });
        }

        running_ = true;
        
        // Main server loop
        while (running_)
        {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd_, &read_fds);
            
            struct timeval timeout;
            timeout.tv_sec = 1;  // 1 second timeout
            timeout.tv_usec = 0;
            
            int select_result = select(server_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (select_result < 0)
            {
                if (errno == EINTR) continue; // Interrupted by signal, continue
                return unexpected(Error{
                    ErrorCode::NetlinkConnectError,
                    format("Select failed: {}", strerror(errno))
                });
            }
            
            if (select_result == 0) continue; // Timeout, check running_ again
            
            if (!FD_ISSET(server_fd_, &read_fds)) continue;

            int client_fd = accept(server_fd_, nullptr, nullptr);
            if (client_fd == -1)
            {
                if (running_) // Only report error if we're still supposed to be running
                {
                    return unexpected(Error{
                        ErrorCode::NetlinkConnectError,
                        format("Failed to accept client connection: {}", strerror(errno))
                    });
                }
                break;
            }

            // Handle client request
            (void)handle_client(client_fd);
            close(client_fd);
        }

        return {};
    }

    void Daemon::stop() noexcept
    {
        running_ = false;
        if (server_fd_ != -1)
        {
            close(server_fd_);
            server_fd_ = -1;
        }
        if (!socket_path_.empty())
        {
            unlink(socket_path_.c_str());
        }
    }

    tl::expected<void, Error> Daemon::handle_client(int client_fd) noexcept
    {
        // Read operation type
        uint8_t op_byte;
        if (recv(client_fd, &op_byte, 1, 0) != 1)
        {
            return send_response(client_fd, false, Error{
                ErrorCode::NetlinkConnectError, "Failed to read operation type"
            });
        }

        DaemonOperation op = static_cast<DaemonOperation>(op_byte);
        
        // Read interface name
        auto interface_name_result = read_interface_name(client_fd);
        if (!interface_name_result)
        {
            return send_response(client_fd, false, interface_name_result.error());
        }

        const std::string& interface_name = interface_name_result.value();

        // Execute requested operation
        tl::expected<void, Error> result;
        switch (op)
        {
            case DaemonOperation::CREATE_VCAN:
                result = create_vcan_interface_if_not_exists(interface_name);
                break;
            case DaemonOperation::INTERFACE_UP:
                result = set_interface_up(interface_name);
                break;
            case DaemonOperation::INTERFACE_DOWN:
                result = set_interface_down(interface_name);
                break;
            default:
                result = unexpected(Error{
                    ErrorCode::NetlinkConnectError, "Unknown operation type"
                });
        }

        // Send response
        if (result)
        {
            return send_response(client_fd, true);
        }
        else
        {
            return send_response(client_fd, false, result.error());
        }
    }

    tl::expected<void, Error> Daemon::create_vcan_interface_if_not_exists(string_view interface_name) noexcept
    {
        // This is moved from VCAN.cpp
        if (if_nametoindex(interface_name.data()) != 0)
        {
            return {}; // already exist.
        }
        if (errno != ENODEV) // if_nametoindex failed for a reason other than "No such device"
        {
            return unexpected(Error{
                ErrorCode::VCANCheckingError, format("Error checking interface '{}' before creation: {}",
                                                     interface_name,
                                                     strerror(errno))
            });
        }

        // Creating VCAN for Interface.
        nl_sock* sock;
        rtnl_link* link;
        if (sock = nl_socket_alloc(); !sock)
        {
            return unexpected(Error{ErrorCode::NlSocketAllocError, "Error: Cannot alloc nl_socket."});
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            return unexpected(Error{ErrorCode::NlConnectError, "Error: Cannot connected to nl_socket."});
        }

        if (link = rtnl_link_alloc(); !link)
        {
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{ErrorCode::RtnlLinkAllocError, "Error: Cannot alloc rtnl_link."});
        }

        rtnl_link_set_name(link, interface_name.data());
        rtnl_link_set_type(link, "vcan");

        if (const int err = rtnl_link_add(sock, link, NLM_F_CREATE); err < 0)
        {
            rtnl_link_put(link);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::RtnlLinkAddError, format("Error: Cannot create interface: {}.", nl_geterror(err))
            });
        }
        rtnl_link_put(link);
        nl_close(sock);
        nl_socket_free(sock);
        return {};
    }

    tl::expected<void, Error> Daemon::set_interface_up(string_view interface_name) noexcept
    {
        // This is moved from Netlink.cpp set_sock<true>()
        nl_sock* sock = nl_socket_alloc();
        if (!sock)
        {
            return unexpected(Error{
                ErrorCode::NetlinkSocketNotInitialized, format("Netlink socket not initialized for {}", interface_name)
            });
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, format("Failed to connect to netlink socket for {}", interface_name)
            });
        }

        const int ifindex = static_cast<int>(if_nametoindex(interface_name.data()));
        if (ifindex == 0)
        {
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkInterfaceNotFound, format("Interface {} not found", interface_name)
            });
        }
        
        rtnl_link* link = rtnl_link_alloc();
        rtnl_link* change = rtnl_link_alloc();
        if (!link || !change)
        {
            if (link) rtnl_link_put(link);
            if (change) rtnl_link_put(change);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::RtnlLinkAllocError, format("Failed to allocate rtnl_link for {}", interface_name)
            });
        }

        rtnl_link_set_ifindex(link, ifindex);
        rtnl_link_set_flags(change, IFF_UP);

        if (const int err = rtnl_link_change(sock, link, change, 0); err < 0)
        {
            rtnl_link_put(link);
            rtnl_link_put(change);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkBringUpError,
                format("Failed to bring up interface {}: {}", interface_name, nl_geterror(err))
            });
        }
        rtnl_link_put(link);
        rtnl_link_put(change);
        nl_close(sock);
        nl_socket_free(sock);
        return {};
    }

    tl::expected<void, Error> Daemon::set_interface_down(string_view interface_name) noexcept
    {
        // This is moved from Netlink.cpp set_sock<false>()
        nl_sock* sock = nl_socket_alloc();
        if (!sock)
        {
            return unexpected(Error{
                ErrorCode::NetlinkSocketNotInitialized, format("Netlink socket not initialized for {}", interface_name)
            });
        }

        if (nl_connect(sock, NETLINK_ROUTE) < 0)
        {
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, format("Failed to connect to netlink socket for {}", interface_name)
            });
        }

        const int ifindex = static_cast<int>(if_nametoindex(interface_name.data()));
        if (ifindex == 0)
        {
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkInterfaceNotFound, format("Interface {} not found", interface_name)
            });
        }
        
        rtnl_link* link = rtnl_link_alloc();
        rtnl_link* change = rtnl_link_alloc();
        if (!link || !change)
        {
            if (link) rtnl_link_put(link);
            if (change) rtnl_link_put(change);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::RtnlLinkAllocError, format("Failed to allocate rtnl_link for {}", interface_name)
            });
        }

        rtnl_link_set_ifindex(link, ifindex);
        rtnl_link_unset_flags(change, IFF_UP);

        if (const int err = rtnl_link_change(sock, link, change, 0); err < 0)
        {
            rtnl_link_put(link);
            rtnl_link_put(change);
            nl_close(sock);
            nl_socket_free(sock);
            return unexpected(Error{
                ErrorCode::NetlinkBringDownError,
                format("Failed to bring down interface {}: {}", interface_name, nl_geterror(err))
            });
        }
        rtnl_link_put(link);
        rtnl_link_put(change);
        nl_close(sock);
        nl_socket_free(sock);
        return {};
    }

    tl::expected<void, Error> Daemon::send_response(int client_fd, bool success) noexcept
    {
        uint8_t success_byte = success ? 1 : 0;
        if (send(client_fd, &success_byte, 1, 0) != 1)
        {
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to send response success flag"
            });
        }
        return {};
    }

    tl::expected<void, Error> Daemon::send_response(int client_fd, bool success, const Error& error) noexcept
    {
        uint8_t success_byte = success ? 1 : 0;
        if (send(client_fd, &success_byte, 1, 0) != 1)
        {
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to send response success flag"
            });
        }

        if (!success)
        {
            // Send error code
            uint8_t error_code = static_cast<uint8_t>(error.code);
            if (send(client_fd, &error_code, 1, 0) != 1)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkConnectError, "Failed to send error code"
                });
            }

            // Send error message length
            uint16_t msg_len = static_cast<uint16_t>(std::min(error.message.length(), static_cast<size_t>(MAX_ERROR_MESSAGE_LEN)));
            if (send(client_fd, &msg_len, 2, 0) != 2)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkConnectError, "Failed to send error message length"
                });
            }

            // Send error message
            if (msg_len > 0 && send(client_fd, error.message.c_str(), msg_len, 0) != msg_len)
            {
                return unexpected(Error{
                    ErrorCode::NetlinkConnectError, "Failed to send error message"
                });
            }
        }

        return {};
    }

    tl::expected<std::string, Error> Daemon::read_interface_name(int client_fd) noexcept
    {
        // Read interface name length
        uint8_t name_len;
        if (recv(client_fd, &name_len, 1, 0) != 1)
        {
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to read interface name length"
            });
        }

        if (name_len == 0 || name_len > MAX_INTERFACE_NAME_LEN)
        {
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Invalid interface name length"
            });
        }

        // Read interface name
        std::string interface_name(name_len, '\0');
        if (recv(client_fd, interface_name.data(), name_len, 0) != name_len)
        {
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to read interface name"
            });
        }

        return interface_name;
    }

    // DaemonClient implementation
    DaemonClient::DaemonClient(const std::string& socket_path) : socket_path_(socket_path) {}

    DaemonClient::~DaemonClient() = default;

    tl::expected<void, Error> DaemonClient::create_vcan_interface(string_view interface_name) noexcept
    {
        return send_request(DaemonOperation::CREATE_VCAN, interface_name);
    }

    tl::expected<void, Error> DaemonClient::set_interface_up(string_view interface_name) noexcept
    {
        return send_request(DaemonOperation::INTERFACE_UP, interface_name);
    }

    tl::expected<void, Error> DaemonClient::set_interface_down(string_view interface_name) noexcept
    {
        return send_request(DaemonOperation::INTERFACE_DOWN, interface_name);
    }

    tl::expected<void, Error> DaemonClient::send_request(DaemonOperation op, string_view interface_name) noexcept
    {
        // Create socket
        int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd == -1)
        {
            return unexpected(Error{
                ErrorCode::NetlinkSocketNotInitialized,
                format("Failed to create client socket: {}", strerror(errno))
            });
        }

        // Connect to daemon
        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (connect(client_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError,
                format("Failed to connect to daemon: {}", strerror(errno))
            });
        }

        // Send operation type
        uint8_t op_byte = static_cast<uint8_t>(op);
        if (send(client_fd, &op_byte, 1, 0) != 1)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to send operation type"
            });
        }

        // Send interface name length
        uint8_t name_len = static_cast<uint8_t>(std::min(interface_name.length(), static_cast<size_t>(MAX_INTERFACE_NAME_LEN)));
        if (send(client_fd, &name_len, 1, 0) != 1)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to send interface name length"
            });
        }

        // Send interface name
        if (name_len > 0 && send(client_fd, interface_name.data(), name_len, 0) != name_len)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to send interface name"
            });
        }

        // Read response
        uint8_t success_byte;
        if (recv(client_fd, &success_byte, 1, 0) != 1)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to read response"
            });
        }

        if (success_byte == 1)
        {
            close(client_fd);
            return {};
        }

        // Read error response
        uint8_t error_code;
        if (recv(client_fd, &error_code, 1, 0) != 1)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to read error code"
            });
        }

        uint16_t msg_len;
        if (recv(client_fd, &msg_len, 2, 0) != 2)
        {
            close(client_fd);
            return unexpected(Error{
                ErrorCode::NetlinkConnectError, "Failed to read error message length"
            });
        }

        std::string error_message;
        if (msg_len > 0)
        {
            error_message.resize(msg_len);
            if (recv(client_fd, error_message.data(), msg_len, 0) != msg_len)
            {
                close(client_fd);
                return unexpected(Error{
                    ErrorCode::NetlinkConnectError, "Failed to read error message"
                });
            }
        }

        close(client_fd);
        return unexpected(Error{
            static_cast<ErrorCode>(error_code), error_message
        });
    }
}