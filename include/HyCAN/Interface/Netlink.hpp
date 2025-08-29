#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <string>
#include <memory>

#include <tl/expected.hpp>

#include "HyCAN/Util/Error.hpp"

namespace HyCAN
{
    class DaemonClient; // Forward declaration

    class Netlink
    {
    public:
        explicit Netlink(std::string_view interface_name);
        ~Netlink(); // Need explicit destructor for unique_ptr with incomplete type
        Netlink() = delete;
        Netlink(const Netlink&) = delete;
        Netlink& operator=(const Netlink&) = delete;
        Netlink(Netlink&&) = delete;
        Netlink& operator=(Netlink&&) = delete;
        
        tl::expected<void, Error> up() noexcept;
        tl::expected<void, Error> down() noexcept;

    private:
        std::string interface_name;
        std::unique_ptr<DaemonClient> daemon_client_;
    };
}

#endif //NETLINK_HPP
