module;
#include <string_view>
#include <format>
#include <xtr/logger.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <net/if.h>

export module HyCAN.Interface.Reaper:Socket;
import HyCAN.Interface.Logger;

using std::string_view;
using xtr::sink;

export namespace HyCAN
{
    class Socket
    {
    public:
        explicit Socket(string_view interface_name);
        Socket() = delete;
        ~Socket();

    private:
        string_view interface_name;
        int sock_fd{};
        sink s;
    };

    Socket::Socket(string_view interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Socket_{}", interface_name));
        sock_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (sock_fd == -1)
        {
            XTR_LOGL(fatal, s, "Failed to create CAN socket");
        }
        ifreq ifr{};
        std::strncpy(ifr.ifr_name, interface_name.data(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) == -1)
        {
            close(sock_fd);
            XTR_LOGL(fatal, s, "Failed to get CAN interface index");
        }

        sockaddr_can addr = {
            .can_family = AF_CAN,
            .can_ifindex = ifr.ifr_ifindex,
        };
        if (bind(sock_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            close(sock_fd);
            XTR_LOGL(fatal, s, "Failed to bind CAN socket");
        }
        const int flags = fcntl(sock_fd, F_GETFL, 0);
        fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
    }

    Socket::~Socket()
    {
        close(sock_fd);
    }
}

