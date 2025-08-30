#ifndef HYCAN_DAEMON_NETLINK_MANAGER_HPP
#define HYCAN_DAEMON_NETLINK_MANAGER_HPP

#include <string_view>
#include <cstdint>

struct nl_sock;
struct nl_cache;

namespace HyCAN
{
    struct NetlinkRequest;
    struct NetlinkResponse;

    /**
     * @brief Manages netlink operations for network interface management
     * 
     * This class encapsulates all libnl3 functionality to reduce coupling
     * with the main Daemon class.
     */
    class NetlinkManager
    {
    private:
        nl_sock* nl_socket_{nullptr};
        nl_cache* link_cache_{nullptr};

        // Private netlink operation methods
        NetlinkResponse set_interface_state_libnl(std::string_view interface_name, bool up) const;
        NetlinkResponse set_can_bitrate_libnl(std::string_view interface_name, uint32_t bitrate) const;

    public:
        NetlinkManager();
        ~NetlinkManager();

        // Initialization and cleanup
        int initialize();
        void cleanup();

        // Public interface query methods
        NetlinkResponse check_interface_exists(std::string_view interface_name) const;
        NetlinkResponse check_interface_is_up(std::string_view interface_name) const;
        
        // Main request processing method
        NetlinkResponse process_request(const NetlinkRequest& request) const;

        // Non-copyable and non-movable
        NetlinkManager(const NetlinkManager&) = delete;
        NetlinkManager& operator=(const NetlinkManager&) = delete;
        NetlinkManager(NetlinkManager&&) = delete;
        NetlinkManager& operator=(NetlinkManager&&) = delete;
    };

} // namespace HyCAN

#endif // HYCAN_DAEMON_NETLINK_MANAGER_HPP