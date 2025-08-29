#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <string>

#include <tl/expected.hpp>

#include "HyCAN/Util/Error.hpp"

namespace HyCAN
{
    class Netlink
    {
    public:
        explicit Netlink(std::string_view interface_name);
        Netlink() = delete;
        tl::expected<void, Error> up() noexcept;
        tl::expected<void, Error> down() noexcept;
        tl::expected<bool, Error> is_up() noexcept;

    private:
        template <bool state>
        tl::expected<void, Error> set_sock() noexcept;
        std::string_view interface_name;
    };
}

#endif //NETLINK_HPP
