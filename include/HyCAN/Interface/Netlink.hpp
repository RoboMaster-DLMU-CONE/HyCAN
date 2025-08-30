#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <string>
#include <memory>
#include <cstdint>

#include <tl/expected.hpp>

#include "HyCAN/Util/Error.hpp"

namespace HyCAN
{
    /**
     * @brief Singleton Netlink manager for HyCAN applications
     * Handles two-stage IPC communication with HyCAN daemon
     */
    class Netlink
    {
    private:
        std::string main_channel_name_;
        std::string client_channel_name_;
        std::unique_ptr<class NetlinkClient> client_;
        bool initialized_{false};
        
        Netlink();
        
        tl::expected<void, Error> ensure_initialized();

    public:
        static Netlink& instance();
        
        // Core interface operations
        tl::expected<void, Error> set(std::string_view interface_name, bool up, uint32_t bitrate = 1000000);
        tl::expected<bool, Error> exists(std::string_view interface_name);
        tl::expected<bool, Error> is_up(std::string_view interface_name);
        tl::expected<void, Error> create_vcan(std::string_view interface_name);
        
        ~Netlink();
        
        // Delete copy constructor and assignment
        Netlink(const Netlink&) = delete;
        Netlink& operator=(const Netlink&) = delete;
    };
}

#endif //NETLINK_HPP
