#ifndef HYCAN_DAEMON_CLASS_HPP
#define HYCAN_DAEMON_CLASS_HPP

#include <atomic>
#include <string_view>

#include "libipc/ipc.h"

struct nl_sock;
struct nl_cache;

namespace HyCAN
{
    struct NetlinkRequest;
    struct NetlinkResponse;

    /**
     * @brief HyCAN Daemon class for handling network interface management
     */
    class Daemon
    {
        std::atomic<bool> running_{true};
        ipc::channel channel_{"HyCAN_Daemon", ipc::receiver};

        nl_sock* nl_socket_{nullptr};
        nl_cache* link_cache_{nullptr};


        int init_netlink();
        void cleanup_netlink();

        NetlinkResponse set_interface_state_libnl(std::string_view interface_name, bool up) const;
        NetlinkResponse set_can_bitrate_libnl(std::string_view interface_name, uint32_t bitrate) const;
        NetlinkResponse check_interface_state_libnl(std::string_view interface_name) const;
        NetlinkResponse validate_can_hardware_libnl(std::string_view interface_name) const;
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
