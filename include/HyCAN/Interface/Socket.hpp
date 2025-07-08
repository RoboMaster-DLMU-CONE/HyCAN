#ifndef HYCAN_SOCKET_HPP
#define HYCAN_SOCKET_HPP

#include <string>

#include <tl/expected.hpp>

namespace HyCAN
{
    class Socket
    {
    public:
        explicit Socket(std::string_view interface_name);
        Socket() = delete;
        ~Socket();
        tl::expected<void, std::string> ensure_connected() noexcept;
        [[nodiscard]] tl::expected<void, std::string> flush() const noexcept;
        int sock_fd{};

    private:
        std::string_view interface_name;
    };
}

#endif //HYCAN_SOCKET_HPP
