#ifndef HYCAN_SOCKET_HPP
#define HYCAN_SOCKET_HPP

#include <string>

#include <HyCAN/Util/Error.hpp>
#include <tl/expected.hpp>
#include <unistd.h>

namespace HyCAN {
class Socket {
  public:
    explicit Socket(std::string_view interface_name);
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    Socket(Socket &&other) noexcept
        : sock_fd(other.sock_fd), interface_name(other.interface_name) {
        other.sock_fd = -1;
    }
    Socket &operator=(Socket &&other) noexcept {
        if (this != &other) {
            if (sock_fd > 0)
                close(sock_fd);
            sock_fd = other.sock_fd;
            interface_name = other.interface_name;
            other.sock_fd = -1;
        }
        return *this;
    }
    Socket() = delete;
    ~Socket();
    tl::expected<void, Error> ensure_connected() noexcept;
    [[nodiscard]] tl::expected<void, Error> flush() const noexcept;

    [[nodiscard]] int get_sock_fd() const { return sock_fd; }

    [[nodiscard]] std::string_view get_interface_name() const {
        return interface_name;
    }

  private:
    int sock_fd{};
    std::string_view interface_name;
};
} // namespace HyCAN

#endif // HYCAN_SOCKET_HPP
