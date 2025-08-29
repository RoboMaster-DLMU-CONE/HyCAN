#ifndef HYCAN_DAEMON_CLASS_HPP
#define HYCAN_DAEMON_CLASS_HPP

#include <atomic>
#include <string_view>
#include <memory>

#include "libipc/ipc.h"

struct nl_sock;
struct nl_cache;

namespace HyCAN
{
    struct NetlinkRequest;
    struct NetlinkResponse;
    class InterfaceManager;
    class CANManager;
    class RequestProcessor;

    /**
     * @brief HyCAN Daemon class for handling network interface management
     */
    class Daemon
    {
        std::atomic<bool> running_{true};
        ipc::channel channel_{"HyCAN_Daemon", ipc::receiver};

        nl_sock* nl_socket_{nullptr};
        nl_cache* link_cache_{nullptr};

        // Modular components for different responsibilities
        std::unique_ptr<InterfaceManager> interface_manager_;
        std::unique_ptr<CANManager> can_manager_;
        std::unique_ptr<RequestProcessor> request_processor_;

        int init_netlink();
        void cleanup_netlink();

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
