#ifndef HYCAN_SOCKET_HPP
#define HYCAN_SOCKET_HPP

#include <string>
#include <expected>

using Result = std::expected<void, std::string>;

namespace HyCAN
{
    class Socket
    {
    public:
        explicit Socket(std::string_view interface_name);
        Socket() = delete;
        ~Socket();
        Result ensure_connected() noexcept;
        [[nodiscard]] Result flush() const noexcept;
        int sock_fd{};

    private:
        std::string_view interface_name;
    };
}

#endif //HYCAN_SOCKET_HPP
