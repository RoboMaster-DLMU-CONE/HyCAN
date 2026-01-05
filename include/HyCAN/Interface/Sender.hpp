#ifndef SENDER_HPP
#define SENDER_HPP

#include <cstring>
#include <format>
#include <string_view>
#include <unistd.h>

#include "CanFrameConvertible.hpp"
#include "Socket.hpp"

namespace HyCAN {
class Sender {
  public:
    explicit Sender(std::string_view interface_name);
    Sender() = delete;

    template <CanFrameConvertible T>
    tl::expected<void, Error> send(T frame) noexcept {

        if (socket.get_sock_fd() <= 0) {
            if (auto res = socket.ensure_connected(); !res) {
                return res;
            }
        }
        auto do_write = [&](int fd) -> ssize_t {
            if constexpr (std::is_same_v<T, can_frame>) {
                return write(fd, &frame, sizeof(frame));
            } else {
                const auto cf = static_cast<can_frame>(frame);
                return write(fd, &cf, sizeof(cf));
            }
        };
        ssize_t result = do_write(socket.get_sock_fd());
        if (result != -1)
            return {};

        int current_err = errno;
        bool is_fatal = (current_err == EBADF || current_err == ENETDOWN ||
                         current_err == EPIPE || current_err == ENXIO ||
                         current_err == ENODEV);

        if (is_fatal) {
            // try re-connect
            if (auto res = socket.ensure_connected(); res) {
                // try re-send
                result = do_write(socket.get_sock_fd());
                if (result != -1)
                    return {}; // success
                // re-send failed
                current_err = errno;
            } else {
                // re-connect failed
                return res;
            }
        }
        if (current_err == EAGAIN || current_err == EWOULDBLOCK ||
            current_err == ENOBUFS) {
            return tl::make_unexpected(Error{ErrorCode::CANSocketBufferFull,
                                             "CAN socket buffer full"});
        }
        return tl::make_unexpected(
            Error{ErrorCode::CANSocketWriteError,
                  std::format("Failed to send CAN message: {} (errno: {})",
                              strerror(current_err), current_err)});
    }

  private:
    Socket socket;
    std::string_view interface_name;
};
} // namespace HyCAN

#endif // SENDER_HPP
