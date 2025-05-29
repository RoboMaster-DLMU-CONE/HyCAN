module;
#include <string_view>
#include <linux/can.h>

#include <xtr/logger.hpp>
export module HyCAN.Interface.Sender;
import HyCAN.Interface.CanFrameConvertible;
import HyCAN.Interface.Socket;
import HyCAN.Interface.Logger;

using std::string_view;
using xtr::sink;

export namespace HyCAN
{
    class Sender
    {
    public:
        explicit Sender(string_view interface_name);
        Sender() = delete;
        template <CanFrameConvertiable T>
        void send(T frame);

    private:
        Socket socket;
        string_view interface_name;
        sink s;
    };

    Sender::Sender(const string_view interface_name): socket(interface_name), interface_name(interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Sender_{}", interface_name));
    }

    template <CanFrameConvertiable T = can_frame>
    void Sender::send(T frame)
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
    }
}
