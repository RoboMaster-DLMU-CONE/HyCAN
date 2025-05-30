#include "Interface/Socket.hpp"
#include "Interface/Logger.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <net/if.h>

namespace HyCAN
{
    Socket::Socket(string_view interface_name): interface_name(interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Socket_{}", interface_name));
    }

    Socket::~Socket()
    {
        close(sock_fd);
    }

    bool Socket::ensure_connected()
    {
        if (sock_fd > 0)
        {
            close(sock_fd);
            sock_fd = -1;
        }
        sock_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (sock_fd == -1)
        {
            XTR_LOGL(error, s, "Failed to create CAN socket: {}", strerror(errno));
            return false;
        }
        ifreq ifr{};
        const auto name_len = std::min(interface_name.size(), static_cast<size_t>(IFNAMSIZ - 1));
        std::memcpy(ifr.ifr_name, interface_name.data(), name_len);
        ifr.ifr_name[name_len] = '\0';
        if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) == -1)
        {
            close(sock_fd);
            sock_fd = -1;
            XTR_LOGL(error, s, "Failed to get CAN interface '{}' index: {}", ifr.ifr_ifrn.ifrn_name, strerror(errno));
            return false;
        }

        sockaddr_can addr = {
            .can_family = AF_CAN,
            .can_ifindex = ifr.ifr_ifindex,
        };

        if (bind(sock_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            close(sock_fd);
            sock_fd = -1;
            XTR_LOGL(error, s, "Failed to bind CAN socket: {}", strerror(errno));
            return false;
        }

        const int flags = fcntl(sock_fd, F_GETFL, 0);
        fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
        return true;
    }

    void Socket::flush()
    {
        if (sock_fd < 0)
        {
            XTR_LOGL(error, s, "Cannot flush with invalid socket descriptor");
            return;
        }
        can_frame frame{};
        while (true)
        {
            if (const ssize_t nbytes = read(sock_fd, &frame, sizeof(can_frame)); nbytes > 0)
            {
            }
            else if (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENETDOWN))
            {
                break;
            }
            else
            {
                XTR_LOGL(error, s, "Failed to flush linux buffer: {}", strerror(errno));
            }
        }
    }
}
