#include "HyCAN/Interface/Socket.hpp"

#include <cstring>
#include <fcntl.h>
#include <format>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <net/if.h>

using tl::unexpected, std::format, std::string_view;

namespace HyCAN
{
    Socket::Socket(const string_view interface_name): interface_name(interface_name)
    {
    };

    Socket::~Socket()
    {
        close(sock_fd);
    }

    tl::expected<void, std::string> Socket::ensure_connected() noexcept
    {
        if (sock_fd > 0)
        {
            close(sock_fd);
            sock_fd = -1;
        }
        sock_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (sock_fd == -1)
        {
            return unexpected(format("Failed to create CAN socket: {}", strerror(errno)));
        }
        ifreq ifr{};
        const auto name_len = std::min(interface_name.size(), static_cast<size_t>(IFNAMSIZ - 1));
        std::memcpy(ifr.ifr_name, interface_name.data(), name_len);
        ifr.ifr_name[name_len] = '\0';
        if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) == -1)
        {
            close(sock_fd);
            sock_fd = -1;
            return unexpected(format("Failed to get CAN interface '{}' index: {}", ifr.ifr_ifrn.ifrn_name,
                                     strerror(errno)));
        }

        sockaddr_can addr = {
            .can_family = AF_CAN,
            .can_ifindex = ifr.ifr_ifindex,
        };

        if (bind(sock_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            close(sock_fd);
            sock_fd = -1;
            return unexpected(format("Failed to bind CAN socket: {}", strerror(errno)));
        }

        const int flags = fcntl(sock_fd, F_GETFL, 0);
        fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
        return {};
    }

    tl::expected<void, std::string> Socket::flush() const noexcept
    {
        if (sock_fd < 0)
        {
            return unexpected("Cannot flush with invalid socket descriptor");
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
                return unexpected(format("Failed to flush linux buffer: {}", strerror(errno)));
            }
        }
        return {};
    }
}
