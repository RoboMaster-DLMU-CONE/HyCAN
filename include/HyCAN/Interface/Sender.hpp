#ifndef SENDER_HPP
#define SENDER_HPP

#include <cstring>
#include <format>
#include <string_view>
#include <unistd.h>

#include "CanFrameConvertible.hpp"
#include "Socket.hpp"

namespace HyCAN
{
    class Sender
    {
    public:
        explicit Sender(std::string_view interface_name);
        Sender() = delete;

        template <CanFrameConvertible T>
        Result send(T frame) noexcept
        {
            if (const auto socket_result = socket.ensure_connected(); !socket_result)
            {
                return std::unexpected(socket_result.error());
            }
            ssize_t result;
            if constexpr (std::is_same<T, can_frame>())
            {
                result = write(socket.sock_fd, &frame, sizeof(frame));
            }
            else
            {
                const auto cf = static_cast<can_frame>(frame);
                result = write(socket.sock_fd, &cf, sizeof(cf));
            }
            if (result == -1)
            {
                return std::unexpected(std::format("Failed to send CAN message: {}", strerror(errno)));
            }
            return {};
        };

    private:
        Socket socket;
        std::string_view interface_name;
    };
}

#endif //SENDER_HPP
