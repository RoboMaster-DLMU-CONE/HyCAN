#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <string>

#include <tl/expected.hpp>

namespace HyCAN
{
    class Netlink
    {
    public:
        explicit Netlink(std::string_view interface_name);
        Netlink() = delete;
        tl::expected<void, std::string> up() noexcept;
        tl::expected<void, std::string> down() noexcept;

    private:
        template <bool state>
        tl::expected<void, std::string> set_sock() noexcept;
        std::string_view interface_name;
    };
}

#endif //NETLINK_HPP
