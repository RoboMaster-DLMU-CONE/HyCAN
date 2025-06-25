#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <expected>
#include <string>
#include <format>

namespace HyCAN
{
    class Netlink
    {
        using Result = std::expected<void, std::string>;

    public:
        explicit Netlink(std::string_view interface_name);
        Netlink() = delete;
        Result up() noexcept;
        Result down() noexcept;

    private:
        template <bool state>
        Result set_sock() noexcept;
        std::string_view interface_name;
    };
}

#endif //NETLINK_HPP
