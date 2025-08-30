#ifndef HYCAN_DAEMON_CLASS_HPP
#define HYCAN_DAEMON_CLASS_HPP

#include <atomic>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <chrono>

#include "UnixSocket/UnixSocket.hpp"

struct nl_sock;
struct nl_cache;

namespace HyCAN
{
    struct NetlinkRequest;
    struct NetlinkResponse;
    struct ClientRegisterRequest;
    struct ClientRegisterResponse;

    /**
     * @brief Client session information
     */
    struct ClientSession
    {
        pid_t client_pid;
        std::string channel_name;
        std::unique_ptr<UnixSocket> socket;
        std::unique_ptr<std::thread> worker_thread;
        std::chrono::steady_clock::time_point last_activity;
        std::atomic<bool> running{true};

        ClientSession(pid_t pid, std::string name) 
            : client_pid(pid), channel_name(std::move(name)), 
              last_activity(std::chrono::steady_clock::now()) {}
    };

    /**
     * @brief HyCAN Daemon class for handling network interface management
     */
    class Daemon
    {
        std::atomic<bool> running_{true};
        std::unique_ptr<UnixSocket> main_socket_;
        
        // Client session management
        std::unordered_map<pid_t, std::unique_ptr<ClientSession>> client_sessions_;
        std::mutex sessions_mutex_;
        std::thread cleanup_thread_;
        
        // Netlink management (singleton-like for the daemon)
        nl_sock* nl_socket_{nullptr};
        nl_cache* link_cache_{nullptr};

        // Main daemon methods
        int init_netlink();
        void cleanup_netlink();
        void cleanup_sessions();
        void session_cleanup_worker();
        
        // Client session methods
        std::string generate_client_channel_name(pid_t client_pid);
        void handle_client_registration(const ClientRegisterRequest& request, const std::unique_ptr<UnixSocket>& registration_socket);
        void client_session_worker(ClientSession* session);
        bool is_process_alive(pid_t pid) const;
        
        // Netlink operation methods
        NetlinkResponse set_interface_state_libnl(std::string_view interface_name, bool up) const;
        NetlinkResponse set_can_bitrate_libnl(std::string_view interface_name, uint32_t bitrate) const;
        NetlinkResponse check_interface_exists(std::string_view interface_name) const;
        NetlinkResponse check_interface_is_up(std::string_view interface_name) const;
        NetlinkResponse process_request(const NetlinkRequest& request) const;

    public:
        Daemon();
        ~Daemon();

        int run();
        void stop();

        Daemon(const Daemon&) = delete;
        Daemon& operator=(const Daemon&) = delete;
    };

    void signal_handler(int sig);
} // namespace HyCAN

#endif
