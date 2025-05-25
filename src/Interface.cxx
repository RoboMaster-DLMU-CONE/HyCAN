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

namespace
{
    bool rtattr_add_data(nlmsghdr* nlh, size_t buffer_capacity, const int type, const void* data, size_t data_len)
    {
        const size_t rta_payload_len = data_len;
        const size_t rta_total_len = RTA_LENGTH(rta_payload_len);
        const size_t rta_aligned_total_len = RTA_ALIGN(rta_total_len);
        if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_aligned_total_len > buffer_capacity)
        {
            return false;
        }
        auto* rta = reinterpret_cast<rtattr*>(reinterpret_cast<char*>(nlh) + NLMSG_ALIGN(nlh->nlmsg_len));
        rta->rta_type = type;
        rta->rta_len = rta_total_len;
        if (rta_payload_len > 0 && data != nullptr)
        {
            memcpy(RTA_DATA(rta), data, rta_payload_len);
        }
        nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_aligned_total_len;
        return true;
    }

    rtattr* rtattr_nest_begin(nlmsghdr* nlh, size_t buffer_capacity, int type)
    {
        constexpr size_t rta_payload_len = 0;
        constexpr size_t rta_total_len = RTA_LENGTH(rta_payload_len);
        constexpr size_t rta_aligned_total_len = RTA_ALIGN(rta_total_len);

        if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_aligned_total_len > buffer_capacity)
        {
            return nullptr;
        }

        auto* rta_container = reinterpret_cast<struct rtattr*>(reinterpret_cast<char*>(nlh) + NLMSG_ALIGN(
            nlh->nlmsg_len));
        rta_container->rta_type = type;
        rta_container->rta_len = rta_total_len;
        nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_aligned_total_len;

        return rta_container;
    }


    void rtattr_nest_end(nlmsghdr* nlh, rtattr* rta_container_start)
    {
        rta_container_start->rta_len = (reinterpret_cast<char*>(nlh) + NLMSG_ALIGN(nlh->nlmsg_len)) - reinterpret_cast<
            char*>(rta_container_start);
    }
}


expected<void, string> create_vcan_interface_if_not_exists(const string_view interface_name)
{
    if (if_nametoindex(interface_name.data()) != 0)
    {
        return {};
    }
    if (errno != ENODEV)
    {
        return unexpected(format("Error checking interface '{}': {}", interface_name, strerror(errno)));
    }

    // 2. Create Netlink socket
    const int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0)
    {
        return unexpected(format("Error: Cannot open Netlink socket to create interface '{}': {}",
                                 interface_name, strerror(errno)));
    }

    // 3. Prepare Netlink message
    struct
    {
        nlmsghdr nlh;
        ifinfomsg ifm;
        char attrbuf[512];
    } req{};

    req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nlh.nlmsg_type = RTM_NEWLINK;
    // NLM_F_CREATE: create if it doesn't exist.
    // NLM_F_EXCL: fail if it does exist (good for ensuring atomicity if we didn't check above).
    // NLM_F_ACK: request an acknowledgement.
    req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
    req.nlh.nlmsg_seq = 1; // Sequence number
    req.nlh.nlmsg_pid = static_cast<__u32>(getpid());

    req.ifm.ifi_family = AF_UNSPEC;

    if (!rtattr_add_data(&req.nlh, sizeof(req), IFLA_IFNAME, interface_name.data(), interface_name.length() + 1))
    {
        close(sock);
        return unexpected(format("Error: Not enough buffer space for IFLA_IFNAME for '{}'", interface_name));
    }

    rtattr* linkinfo = rtattr_nest_begin(&req.nlh, sizeof(req), IFLA_LINKINFO);
    if (!linkinfo)
    {
        close(sock);
        return unexpected(format("Error: Not enough buffer space for IFLA_LINKINFO for '{}'", interface_name));
    }

    if (const auto kind = "vcan"; !rtattr_add_data(&req.nlh, sizeof(req), IFLA_INFO_KIND, kind, strlen(kind) + 1))
    {
        rtattr_nest_end(&req.nlh, linkinfo);
        close(sock);
        return unexpected(format("Error: Not enough buffer space for IFLA_INFO_KIND for '{}'", interface_name));
    }

    rtattr_nest_end(&req.nlh, linkinfo);


    if (send(sock, &req.nlh, req.nlh.nlmsg_len, 0) < 0)
    {
        const int send_errno = errno;
        close(sock);
        if (send_errno == EEXIST)
        {
            if (if_nametoindex(interface_name.data()) != 0)
            {
                return {};
            }
        }
        return unexpected(format("Error: Failed to send Netlink message to create interface '{}': {}",
                                 interface_name, strerror(send_errno)));
    }

    close(sock);
    return {};
}

export enum class CANInterfaceType
{
    Virtual,
    Physics
};

using enum CANInterfaceType;

export template <CANInterfaceType type>
expected<void, string> set_interface_up(const std::string_view interface_name)
{
    if constexpr (type == Virtual)
    {
        if (auto result = create_vcan_interface_if_not_exists(interface_name); !result)
        {
            return result;
        }
    }
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
