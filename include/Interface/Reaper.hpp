#ifndef REAPER_HPP
#define REAPER_HPP

#include <thread>

#include <xtr/logger.hpp>

#include "Socket.hpp"

using std::string_view, std::format, std::jthread, std::stop_token;
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
        Reaper& operator=(const Reaper& other) = delete;
        Reaper& operator=(Reaper&& other) noexcept = delete;

        void start();
        void stop();

    private:
        void reap_process(const stop_token& stop_token);
        void epoll_fd_add_sock_fd(int sock_fd);
        Socket socket;
        int thread_event_fd{};
        int epoll_fd{};
        sink s;
        string_view interface_name;
        jthread reap_thread;
    };
}

#endif //REAPER_HPP
