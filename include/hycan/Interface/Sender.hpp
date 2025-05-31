#ifndef SENDER_HPP
#define SENDER_HPP

#include <string_view>

#include <xtr/logger.hpp>

#include "CanFrameConvertible.hpp"
#include "Socket.hpp"

using std::string_view;
using xtr::sink;

namespace HyCAN
{
    class Sender
    {
    public:
        explicit Sender(string_view interface_name);
        Sender() = delete;

        template <CanFrameConvertible T>
        void send(T frame)
        {
            socket.ensure_connected();
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
                XTR_LOGL(error, s, "Failed to send CAN message: {}", strerror(errno));
            }
        };

    private:
        Socket socket;
        string_view interface_name;
        sink s;
    };
}

#endif //SENDER_HPP
