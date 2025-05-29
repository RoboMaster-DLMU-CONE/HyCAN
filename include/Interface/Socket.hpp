#ifndef HYCAN_SOCKET_HPP
#define HYCAN_SOCKET_HPP

#include <string_view>
#include <format>
#include <xtr/logger.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <net/if.h>

using std::string_view;
using xtr::sink;

namespace HyCAN
{
    class Socket
    {
    public:
        explicit Socket(string_view interface_name);
        Socket() = delete;
        ~Socket();
        bool ensure_connected();
        void flush();
        int sock_fd{};

    private:
        string_view interface_name;
        sink s;
    };
}

#endif //HYCAN_SOCKET_HPP
