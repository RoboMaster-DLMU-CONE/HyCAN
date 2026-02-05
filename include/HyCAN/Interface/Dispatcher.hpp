#ifndef REAPER_HPP
#define REAPER_HPP

#include <concepts>
#include <format>
#include <functional>
#include <set>
#include <string>
#include <thread>

#include <tl/expected.hpp>

#include <linux/can.h>
#include <type_traits>

#include <optional>

#include "CanFrameConvertible.hpp"
#include "HyCAN/Util/SpinLock.hpp"
#include "Socket.hpp"

static constexpr size_t MAX_EPOLL_EVENT = 2048;

namespace HyCAN {
class Dispatcher {
  public:
    explicit Dispatcher(std::string_view interface_name, const std::optional<uint8_t>& cpu_core_opt = std::nullopt);
    Dispatcher() = delete;
    Dispatcher(const Dispatcher &other) = delete;
    Dispatcher(Dispatcher &&other) = delete;
    ~Dispatcher();
    Dispatcher &operator=(const Dispatcher &other) = delete;
    Dispatcher &operator=(Dispatcher &&other) noexcept = delete;

    tl::expected<void, Error> start() noexcept;
    tl::expected<void, Error> stop() noexcept;

    template <typename T = can_frame, typename Func>
        requires(CanFrameConvertible<T> && std::invocable<Func, T>)
    tl::expected<void, Error> register_func(const std::set<size_t> &can_ids,
                                            Func &&func) {

        std::function<void(can_frame)> register_func;
        register_func = [func = std::forward<decltype(func)>(func)](
                            can_frame frame) mutable {
            func(static_cast<T>(frame));
        };
        lock_.lock();
        for (auto id : can_ids) {
            if (id >= 2048) {
                return tl::unexpected(
                    Error{ErrorCode::FuncCANIdSetError,
                          std::format("CAN ID {} exceeds maximum limit of 2047",
                                      id)});
            }
            funcs[id] = register_func;
        }
        lock_.unlock();
        return {};
    }

#ifdef HYCAN_LATENCY_TEST
    struct LatencyStats {
        uint64_t total_latency_ns = 0;
        uint64_t message_count = 0;
        double average_latency_us = 0.0;
    };

    LatencyStats get_latency_stats() const;
#endif

  private:
    void reap_process(const std::stop_token &stop_token);
    tl::expected<void, Error> epoll_fd_add_sock_fd(int sock_fd) const noexcept;

    Socket socket;
    int thread_event_fd{-1};
    int epoll_fd{-1};
    uint8_t cpu_core{};
    std::function<void(can_frame)> funcs[HC_MAX_STD_CAN_ID]{};
    std::string_view interface_name;
    std::jthread reap_thread;
    Util::SpinLock lock_;

#ifdef HYCAN_LATENCY_TEST
    mutable std::atomic<uint64_t> accumulated_latency_ns{0};
    mutable std::atomic<uint64_t> latency_message_count{0};
#endif
};
} // namespace HyCAN

#endif // REAPER_HPP
