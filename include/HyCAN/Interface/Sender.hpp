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
        tl::expected<void, Error> send(T frame) noexcept
        {
            return socket.validate_connection().and_then([&]
            {
                ssize_t result;
                if constexpr (std::is_same<T, can_frame>())
                {
                    result = write(socket.get_sock_fd(), &frame, sizeof(frame));
                }
                else
                {
                    const auto cf = static_cast<can_frame>(frame);
                    result = write(socket.get_sock_fd(), &cf, sizeof(cf));
                }
                if (result == -1)
                {
                    return tl::expected<void, Error>(tl::make_unexpected(Error{
                        ErrorCode::CANSocketWriteError,
                        std::format(
                            "Failed to send CAN message: {}",
                            strerror(errno))
                    }));
                }
                return tl::expected<void, Error>{};
            });
        };

    private:
        Socket socket;
        std::string_view interface_name;
    };
}

#endif //SENDER_HPP
