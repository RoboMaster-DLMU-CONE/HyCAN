#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <xtr/logger.hpp>

using std::format, std::string_view, std::string;
using xtr::logger, xtr::sink;

namespace HyCAN
{
    class Netlink
    {
    public:
        explicit Netlink(string_view interface_name);
        Netlink() = delete;
        void up();
        void down();

    private:
        template <bool state>
        void set_sock();
        string_view interface_name;
        sink s;
    };
}

#endif //NETLINK_HPP
