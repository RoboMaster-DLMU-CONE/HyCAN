#ifndef REAPER_HPP
#define REAPER_HPP

#include <format>
#include <thread>
#include <functional>
#include <string>
#include <set>

#include <linux/can.h>

#include "Socket.hpp"
#include "CanFrameConvertible.hpp"
#include "HyCAN/Util/SpinLock.hpp"

static constexpr size_t MAX_EPOLL_EVENT = 2048;

namespace HyCAN
{
    class Reaper
    {
        using Result = std::expected<void, std::string>;

    public:
        explicit Reaper(std::string_view interface_name);
        Reaper() = delete;
        Reaper(const Reaper& other) = delete;
        Reaper(Reaper&& other) = delete;
        ~Reaper();
        Reaper& operator=(const Reaper& other) = delete;
        Reaper& operator=(Reaper&& other) noexcept = delete;

        Result start() noexcept;
        Result stop() noexcept;

        template <CanFrameConvertible T = can_frame>
        Result tryRegisterFunc(const std::set<size_t>& can_ids, std::function<void(T&&)> func)
        {
            if (!func)
            {
                return std::unexpected("Provided callback function is empty");
            }
            std::function<void(can_frame&&)> register_func;
            if constexpr (std::is_same<T, can_frame>())
            {
                register_func = std::move(func);
            }
            else
            {
                register_func = [&func](can_frame&& frame)
                {
                    func(static_cast<T>(frame));
                };
            }
            lock_.lock();
            for (auto id : can_ids)
            {
                if (id >= 2048)
                {
                    return std::unexpected(std::format("CAN ID {} exceeds maximum limit of 2047", id));
                }
                funcs[id] = register_func;
            }
            lock_.unlock();
            return {};
        }

#ifdef HYCAN_LATENCY_TEST
        struct LatencyStats
        {
            uint64_t total_latency_ns = 0;
            uint64_t message_count = 0;
            double average_latency_us = 0.0;
        };

        LatencyStats get_latency_stats() const;
#endif

    private:
        void reap_process(const std::stop_token& stop_token);
        Result epoll_fd_add_sock_fd(int sock_fd) const noexcept;

        Socket socket;
        int thread_event_fd{-1};
        int epoll_fd{-1};
        uint8_t cpu_core{};
        std::function<void(can_frame&&)> funcs[2048]{};
        std::string_view interface_name;
        std::jthread reap_thread;
        Util::SpinLock lock_;

#ifdef HYCAN_LATENCY_TEST
        mutable std::atomic<uint64_t> accumulated_latency_ns{0};
        mutable std::atomic<uint64_t> latency_message_count{0};
#endif
    };
}

#endif //REAPER_HPP
