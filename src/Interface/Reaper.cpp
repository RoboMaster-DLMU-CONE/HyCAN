#include "Interface/Reaper.hpp"
#include "Interface/Logger.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <linux/can.h>

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
        epoll_fd_add_sock_fd(socket.sock_fd);
    }

    void Reaper::start()
    {
        socket.ensure_connected();
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

    void Reaper::reap_process(const stop_token& stop_token)
    {
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
                        XTR_LOGL(info, s, "Message from {}: ", interface_name);
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
