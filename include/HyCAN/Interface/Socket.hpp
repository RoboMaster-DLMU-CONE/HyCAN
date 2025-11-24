#ifndef HYCAN_SOCKET_HPP
#define HYCAN_SOCKET_HPP

#include <string>

#include <tl/expected.hpp>
#include <HyCAN/Util/Error.hpp>

namespace HyCAN
{
    class Socket
    {
    public:
        explicit Socket(std::string_view interface_name);
        Socket() = delete;
        ~Socket();
        tl::expected<void, Error> ensure_connected() noexcept;
        tl::expected<void, Error> validate_connection() noexcept;
        [[nodiscard]] tl::expected<void, Error> flush() const noexcept;

        [[nodiscard]] int get_sock_fd() const
        {
            return sock_fd;
        }

        [[nodiscard]] std::string_view get_interface_name() const
        {
            return interface_name;
        }

    private:
        int sock_fd{};
        std::string_view interface_name;
    };
}

#endif //HYCAN_SOCKET_HPP
