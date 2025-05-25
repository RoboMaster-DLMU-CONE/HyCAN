module;
#include <cstring>
#include <expected>
#include <string>
#include <string_view>
#include <format>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
using std::expected, std::unexpected, std::string, std::string_view, std::format;
export module HyCAN:Interface;

export expected<void, string> set_interface_up(const std::string_view interface_name)
{
    int sock{};
    struct
    {
        nlmsghdr nlh;
        ifinfomsg ifm;
    } req{};

    const unsigned int if_index = if_nametoindex(interface_name.data());
    if (if_index == 0)
    {
        return unexpected(format("Error: Cannot find interface '{}': {}", interface_name, strerror(errno)));
    }
    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0)
    {
        return unexpected(format("Error: Cannot open Netlink socket for interface '{}': {}", interface_name,
                                 strerror(errno)));
    }

    req.nlh = {
        .nlmsg_len = NLMSG_LENGTH(sizeof(ifinfomsg)),
        .nlmsg_type = RTM_NEWLINK,
        .nlmsg_flags = NLM_F_REQUEST,
        .nlmsg_seq = 0,
        .nlmsg_pid = static_cast<__u32>(getpid()),
    };
    req.ifm = {
        .ifi_family = AF_UNSPEC,
        .ifi_index = static_cast<int>(if_index),
        .ifi_flags = IFF_UP,
        .ifi_change = IFF_UP,
    };

    if (send(sock, &req, req.nlh.nlmsg_len, 0) < 0)
    {
        close(sock);
        return unexpected(format("Error: Failed to send Netlink message for interface '{}': {}", interface_name,
                                 strerror(errno)));
    }
    close(sock);
    return {};
}
