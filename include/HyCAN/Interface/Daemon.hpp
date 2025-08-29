#ifndef DAEMON_HPP
#define DAEMON_HPP

#include <string>
#include <string_view>
#include <memory>

#include <tl/expected.hpp>

#include "HyCAN/Util/Error.hpp"

namespace HyCAN
{
    // IPC message types for daemon communication
    enum class DaemonOperation : uint8_t
    {
        CREATE_VCAN = 1,
        INTERFACE_UP = 2,
        INTERFACE_DOWN = 3
    };

    // Daemon class that handles libnl3 operations
    class Daemon
    {
    public:
        Daemon();
        ~Daemon();

        // Start the daemon server
        tl::expected<void, Error> start(const std::string& socket_path = "/tmp/hycan_daemon.sock") noexcept;
        
        // Stop the daemon server
        void stop() noexcept;
        
        // Process a single client request (used internally by server loop)
        tl::expected<void, Error> handle_client(int client_fd) noexcept;

    private:
        // Core netlink operations (moved from Netlink class)
        tl::expected<void, Error> create_vcan_interface_if_not_exists(std::string_view interface_name) noexcept;
        tl::expected<void, Error> set_interface_up(std::string_view interface_name) noexcept;
        tl::expected<void, Error> set_interface_down(std::string_view interface_name) noexcept;
        
        // Common implementation for interface up/down operations
        tl::expected<void, Error> set_interface_state(std::string_view interface_name, bool up) noexcept;

        // Helper methods
        tl::expected<void, Error> send_response(int client_fd, bool success) noexcept;
        tl::expected<void, Error> send_response(int client_fd, bool success, const Error& error) noexcept;
        tl::expected<std::string, Error> read_interface_name(int client_fd) noexcept;

        int server_fd_{-1};
        std::string socket_path_;
        bool running_{false};
    };

    // Client helper functions for communicating with daemon
    class DaemonClient
    {
    public:
        explicit DaemonClient(const std::string& socket_path = "/tmp/hycan_daemon.sock");
        ~DaemonClient();

        tl::expected<void, Error> create_vcan_interface(std::string_view interface_name) noexcept;
        tl::expected<void, Error> set_interface_up(std::string_view interface_name) noexcept;
        tl::expected<void, Error> set_interface_down(std::string_view interface_name) noexcept;

    private:
        tl::expected<void, Error> send_request(DaemonOperation op, std::string_view interface_name) noexcept;
        
        std::string socket_path_;
    };
}

#endif //DAEMON_HPP