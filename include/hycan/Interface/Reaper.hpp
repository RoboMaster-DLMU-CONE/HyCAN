#ifndef REAPER_HPP
#define REAPER_HPP

#include <thread>
#include <functional>
#include <string>
#include <set>

#include <linux/can.h>

#include <xtr/logger.hpp>

#include "Socket.hpp"

using std::string_view, std::function, std::format, std::jthread, std::stop_token,
    std::atomic,
    std::string, std::set;
using xtr::sink, xtr::logger;

static constexpr size_t MAX_EPOLL_EVENT = 2048;

namespace HyCAN
{
    class Reaper
    {
    public:
        explicit Reaper(string_view interface_name);
        Reaper() = delete;
        Reaper(const Reaper& other) = delete;
        Reaper(Reaper&& other) = delete;
        ~Reaper();
        Reaper& operator=(const Reaper& other) = delete;
        Reaper& operator=(Reaper&& other) noexcept = delete;

        void start();
        void stop();

        void tryRegisterFunc(const set<size_t>& can_ids, function<void(can_frame&&)> func);

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
        void reap_process(const stop_token& stop_token);
        void epoll_fd_add_sock_fd(int sock_fd);

        Socket socket;
        int thread_event_fd{-1};
        int epoll_fd{-1};
        uint8_t cpu_core{};
        function<void(can_frame&&)> funcs[2048]{};
        sink s;
        string_view interface_name;
        jthread reap_thread;

#ifdef HYCAN_LATENCY_TEST
        mutable atomic<uint64_t> accumulated_latency_ns{0};
        mutable atomic<uint64_t> latency_message_count{0};
#endif
    };
}

#endif //REAPER_HPP
