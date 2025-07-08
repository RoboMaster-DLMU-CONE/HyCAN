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
        [[nodiscard]] tl::expected<void, Error> flush() const noexcept;
        int sock_fd{};

    private:
        std::string_view interface_name;
    };
}

#endif //HYCAN_SOCKET_HPP
