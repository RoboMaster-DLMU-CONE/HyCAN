#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <string>
#include <memory>

#include <tl/expected.hpp>

#include "HyCAN/Util/Error.hpp"

namespace HyCAN
{
    /**
     * @brief Singleton Netlink manager for HyCAN applications
     * Handles two-stage IPC communication with HyCAN daemon
     */
    class NetlinkManager
    {
    private:
        std::string main_channel_name_;
        std::string client_channel_name_;
        std::unique_ptr<class NetlinkClient> client_;
        bool initialized_{false};
        
        NetlinkManager();
        
        tl::expected<void, Error> ensure_initialized();

    public:
        static NetlinkManager& instance();
        
        // Core interface operations
        tl::expected<void, Error> set(std::string_view interface_name, bool up);
        tl::expected<bool, Error> exists(std::string_view interface_name);
        tl::expected<bool, Error> is_up(std::string_view interface_name);
        tl::expected<void, Error> create_vcan(std::string_view interface_name);
        
        ~NetlinkManager();
        
        // Delete copy constructor and assignment
        NetlinkManager(const NetlinkManager&) = delete;
        NetlinkManager& operator=(const NetlinkManager&) = delete;
    };

    /**
     * @brief Legacy Netlink class for backward compatibility
     */
    class Netlink
    {
    public:
        explicit Netlink(std::string_view interface_name);
        Netlink() = delete;
        tl::expected<void, Error> up() noexcept;
        tl::expected<void, Error> down() noexcept;

    private:
        std::string interface_name_;
    };
}

#endif //NETLINK_HPP
