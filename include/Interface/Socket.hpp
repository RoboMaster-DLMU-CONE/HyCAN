#ifndef HYCAN_SOCKET_HPP
#define HYCAN_SOCKET_HPP

#include <string_view>
#include <xtr/logger.hpp>

using std::string_view;
using xtr::sink;

namespace HyCAN
{
    class Socket
    {
    public:
        explicit Socket(string_view interface_name);
        Socket() = delete;
        ~Socket();
        bool ensure_connected();
        void flush();
        int sock_fd{};

    private:
        string_view interface_name;
        sink s;
    };
}

#endif //HYCAN_SOCKET_HPP
