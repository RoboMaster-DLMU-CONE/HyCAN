#include "HyCAN/Interface/Reaper.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <linux/can.h>

#include <utility>
#include <chrono>
#include <cstring>

static std::atomic<uint8_t> thread_counter;

using std::unexpected, std::format, std::jthread;

inline Result lock_memory()
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
    {
        return unexpected(format("Failed to lock memory: {}", strerror(errno)));
    }
    return {};
}

inline Result make_real_time()
{
    sched_param param{};
    param.sched_priority = 80;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0)
    {
        return unexpected(format("Failed to make thread real time: {}", strerror(errno)));
    }
    return {};
}

inline Result affinize_cpu(const uint8_t cpu)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu, &cpu_set);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) != 0)
    {
        return unexpected(format("Failed to set cpu affinity: {}", strerror(errno)));
    }
    return {};
}

namespace HyCAN
{
    Reaper::Reaper(const std::string_view interface_name): socket(interface_name), interface_name(interface_name)
    {
        epoll_fd = epoll_create(256);
        if (epoll_fd == -1)
        {
            throw std::runtime_error(format("Failed to create epoll file descriptor: {}", strerror(errno)));
        }
        thread_event_fd = eventfd(0, 0);
        if (thread_event_fd == -1)
        {
            throw std::runtime_error(format("Failed to create thread_event_fd file descriptor: {}", strerror(errno)));
        }
        if (const auto result = epoll_fd_add_sock_fd(thread_event_fd); !result)
        {
            throw std::runtime_error(result.error());
        }

        cpu_core = thread_counter.fetch_add(1, std::memory_order_acquire) % get_nprocs();
    }

    Reaper::~Reaper()
    {
        [[maybe_unused]] const auto _ = stop();
        if (epoll_fd != -1)
        {
            close(epoll_fd);
        }
        if (thread_event_fd != -1)
        {
            close(thread_event_fd);
        }
    }

    Result Reaper::start() noexcept
    {
        if (const auto result = socket.ensure_connected(); !result)
        {
            return unexpected(result.error());
        }
        if (const auto result = epoll_fd_add_sock_fd(socket.sock_fd); !result)
        {
            return unexpected(result.error());
        }

        if (const auto result = socket.flush(); !result)
        {
            return unexpected(result.error());
        }

        if (!reap_thread.joinable())
        {
            reap_thread = jthread(&Reaper::reap_process, this);
        }
        return {};
    }

    Result Reaper::stop() noexcept
    {
        if (reap_thread.joinable())
        {
            reap_thread.request_stop();
            constexpr uint64_t one = 1;
            [[maybe_unused]] const ssize_t _ = write(thread_event_fd, &one, sizeof(one));
            reap_thread.join();
        }
        return {};
    }

    void Reaper::reap_process(const std::stop_token& stop_token)
    {
        [[maybe_unused]] auto _ = lock_memory();
        _ = make_real_time();
        _ = affinize_cpu(cpu_core);
        epoll_event events[MAX_EPOLL_EVENT]{};
        while (true)
        {
            const int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENT, -1);
            if (nfds == -1)
            {
                if (stop_token.stop_requested())
                {
                    return;
                }
                throw std::runtime_error(format("Failed to epoll_wait: {}", strerror(errno)));
            }

            for (int i = 0; i < nfds; ++i)
            {
                if (events[i].data.fd == thread_event_fd && stop_token.stop_requested()) return;
                if (events[i].events & EPOLLIN)
                {
                    can_frame frame{};
                    if (const int fd = events[i].data.fd; read(fd, &frame, sizeof
                                                               frame) > 0)
                    {
#ifdef HYCAN_LATENCY_TEST
                        auto receive_time = std::chrono::high_resolution_clock::now();
                        if (frame.len == 8)
                        {
                            uint64_t send_timestamp_ns_count;
                            memcpy(&send_timestamp_ns_count, frame.data, sizeof(send_timestamp_ns_count));
                            auto send_time_epoch_ns = std::chrono::nanoseconds(send_timestamp_ns_count);
                            auto latency_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                receive_time.time_since_epoch() - send_time_epoch_ns);
                            if (latency_duration_ns.count() >= 0)
                            {
                                accumulated_latency_ns.
                                    fetch_add(latency_duration_ns.count(), std::memory_order_relaxed);
                                latency_message_count.fetch_add(1, std::memory_order_relaxed);
                            }
                        }

#endif

                        lock_.lock();
                        if (funcs[frame.can_id])
                        {
                            funcs[frame.can_id](std::move(frame));
                        }
                        lock_.unlock();
                    }
                }
            }
        }
    }

    Result Reaper::epoll_fd_add_sock_fd(const int sock_fd) const noexcept
    {
        epoll_event ev{};
        ev = {
            .events = EPOLLIN,
            .data = {
                .fd = sock_fd
            }
        };
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1)
        {
            return unexpected(format("Failed to EPOLL_CTL_ADD thread_event_fd: {}", strerror(errno)));
        }
        return {};
    }
#ifdef HYCAN_LATENCY_TEST
    Reaper::LatencyStats Reaper::get_latency_stats() const
    {
        LatencyStats stats;
        stats.total_latency_ns = accumulated_latency_ns.load(std::memory_order_relaxed);
        stats.message_count = latency_message_count.load(std::memory_order_relaxed);
        if (stats.message_count > 0)
        {
            stats.average_latency_us = static_cast<double>(stats.total_latency_ns) / static_cast<double>(stats.
                message_count) / 1000.0;
        }
        return stats;
    }
#endif
}
