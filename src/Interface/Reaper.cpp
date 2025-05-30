#include "Interface/Reaper.hpp"
#include "Interface/Logger.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/sysinfo.h>
#include <linux/can.h>

#include <utility>

inline void lock_memory(sink& s)
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
    {
        XTR_LOGL(error, s, "Failed to lock memory: {}", strerror(errno));
    }
}

inline void make_real_time(sink& s)
{
    sched_param param{};
    param.sched_priority = 80;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0)
    {
        XTR_LOGL(error, s, "Failed to make thread real time: {}", strerror(errno));
    }
}

inline void affinize_cpu(const uint8_t cpu, sink& s)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu, &cpu_set);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) != 0)
    {
        XTR_LOGL(error, s, "Failed to set cpu affinity: {}", strerror(errno));
    }
}

namespace HyCAN
{
    Reaper::Reaper(const string_view interface_name): socket(interface_name), interface_name(interface_name)
    {
        s = interface_logger.get_sink(format("ReaperThread_{}", interface_name));

        epoll_fd = epoll_create(256);
        if (epoll_fd == -1)
        {
            XTR_LOGL(fatal, s, "Failed to create epoll file descriptor: {}", strerror(errno));
        }
        thread_event_fd = eventfd(0, 0);
        if (thread_event_fd == -1)
        {
            XTR_LOGL(fatal, s, "Failed to create thread_event_fd file descriptor: {}", strerror(errno));
        }

        epoll_fd_add_sock_fd(thread_event_fd);
        thread_counter++;
        cpu_core = thread_counter % get_nprocs();
    }

    Reaper::~Reaper()
    {
        if (epoll_fd != -1)
        {
            close(epoll_fd);
        }
        if (thread_event_fd != -1)
        {
            close(thread_event_fd);
        }
    }

    void Reaper::start()
    {
        socket.ensure_connected();
        epoll_fd_add_sock_fd(socket.sock_fd);
        socket.flush();
        if (!reap_thread.joinable())
        {
            reap_thread = jthread(&Reaper::reap_process, this);
        }
    }

    void Reaper::stop()
    {
        if (reap_thread.joinable())
        {
            reap_thread.request_stop();
            constexpr uint64_t one = 1;
            [[maybe_unused]] const ssize_t _ = write(thread_event_fd, &one, sizeof(one));
            reap_thread.join();
        }
    }

    expected<void, string> Reaper::registerFunc(const size_t can_id, function<void(can_frame&&)> func) noexcept
    {
        if (can_id >= 2048)
        {
            return unexpected(format("CAN ID {} exceeds maximum limit of 2047", can_id));
        }
        if (!func)
        {
            return unexpected(format("Provided callback function for CAN ID {} is empty", can_id));
        }
        if (reap_thread.joinable())
        {
            return unexpected("Reaper thread is running.");
        }
        funcs[can_id] = std::move(func);
        return {};
    }

    void Reaper::reap_process(const stop_token& stop_token)
    {
        lock_memory(s);
        make_real_time(s);
        affinize_cpu(cpu_core, s);
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
                XTR_LOGL(fatal, s, "Failed to epoll_wait: {}", strerror(errno));
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
                        XTR_LOGL(debug, s, "Message from {}, CAN ID: {}", interface_name, frame.can_id);
                        if (funcs[frame.can_id])
                        {
                            funcs[frame.can_id](std::move(frame));
                        }
                    }
                }
            }
        }
    }

    void Reaper::epoll_fd_add_sock_fd(const int sock_fd)
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
            XTR_LOGL(fatal, s, "Failed to EPOLL_CTL_ADD thread_event_fd: {}", strerror(errno));
        }
    }
}
