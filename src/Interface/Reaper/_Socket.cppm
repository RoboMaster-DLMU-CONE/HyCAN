module;
#include <string_view>
#include <format>
#include <xtr/logger.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>

export module HyCAN.Interface.Reaper:Socket;

using std::string_view;

export namespace HyCAN
{
    class Socket
    {
    public:
        Socket(string_view interface_name);

    private:
        string_view interface_name;
        int sock_fd;
    };
}

