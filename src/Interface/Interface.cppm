module;
#include <cstring>
#include <expected>
#include <string>
#include <string_view>
#include <format>
#include <exception>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#include <xtr/logger.hpp>
export module HyCAN.Interface;
import :VCAN;
using std::expected, std::unexpected, std::string, std::string_view, std::format, std::exception;

export namespace HyCAN
{
    enum class InterfaceType
    {
        Virtual,
        Physics
    };

    using enum InterfaceType;

    template <InterfaceType type>
    class Interface
    {
    public:
        explicit Interface(std::string_view interface_name);
        Interface() = delete;
        expected<void, string> up();
        expected<void, string> down();

    private:
        int sock{};
        std::string_view interface_name;

        struct NetlinkIfInfoRequest
        {
            nlmsghdr nlh;
            ifinfomsg ifm;
        };

        NetlinkIfInfoRequest req{};
        xtr::logger log;
        xtr::sink s;
    };

    template <InterfaceType type>
    Interface<type>::Interface(const std::string_view interface_name): interface_name(interface_name)
    {
        s = log.get_sink(format("HyCAN Interface_{}", interface_name));
        if constexpr (type == Virtual)
        {
            if (auto result = create_vcan_interface_if_not_exists(interface_name); !result)
            {
                XTR_LOGL(error, s, "Error during vcan creation: {}", result.error());
            }
        }
    }

    template <InterfaceType type>
    expected<void, string> Interface<type>::up()
    {
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
            return unexpected(format("Error: Failed to send Netlink message for interface '{}': {}",
                                     interface_name,
                                     strerror(errno)));
        }
        close(sock);
        return {};
    }

    template <InterfaceType type>
    expected<void, string> Interface<type>::down()
    {
        const unsigned int if_index = if_nametoindex(interface_name.data());
        if (if_index == 0)
        {
            return unexpected(format("错误: 找不到要关闭的接口 '{}': {}", interface_name, strerror(errno)));
        }

        sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (sock < 0)
        {
            return unexpected(format("错误: 无法为接口 '{}' 打开Netlink套接字: {}", interface_name, strerror(errno)));
        }

        req.nlh = {
            .nlmsg_len = NLMSG_LENGTH(sizeof(ifinfomsg)),
            .nlmsg_type = RTM_NEWLINK,
            .nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK, // 添加 NLM_F_ACK
            .nlmsg_seq = 3, // 为ACK使用不同的序列号
            .nlmsg_pid = static_cast<__u32>(getpid()),
        };
        req.ifm = {
            .ifi_family = AF_UNSPEC,
            .ifi_index = static_cast<int>(if_index),
            .ifi_flags = 0,
            .ifi_change = IFF_UP,
        };

        if (send(sock, &req.nlh, req.nlh.nlmsg_len, 0) < 0)
        {
            const int send_errno = errno;
            close(sock);
            return unexpected(format("错误: 发送Netlink消息以关闭接口 '{}' 失败: {}", interface_name, strerror(send_errno)));
        }

        // 等待ACK
        char recv_buf[NLMSG_ALIGN(NLMSG_HDRLEN) + NLMSG_ALIGN(sizeof(struct nlmsgerr)) + 256];
        ssize_t len = recv(sock, recv_buf, sizeof(recv_buf), 0);
        if (len < 0)
        {
            const int recv_errno = errno;
            close(sock);
            return unexpected(format("错误: 接收关闭接口 '{}' 的ACK失败: {}", interface_name, strerror(recv_errno)));
        }

        bool ack_success = false;
        for (auto* ack_nlh = reinterpret_cast<nlmsghdr*>(recv_buf); NLMSG_OK(ack_nlh, static_cast<__u32>(len)); ack_nlh
             =
             NLMSG_NEXT(ack_nlh, len))
        {
            if (ack_nlh->nlmsg_pid != static_cast<__u32>(getpid()) || ack_nlh->nlmsg_seq != req.nlh.nlmsg_seq)
            {
                continue;
            }
            if (ack_nlh->nlmsg_type == NLMSG_ERROR)
            {
                const auto* err = static_cast<const nlmsgerr*>(NLMSG_DATA(ack_nlh));
                if (err->error == 0)
                {
                    ack_success = true;
                    break;
                }
                close(sock);
                return unexpected(format("错误: Netlink ACK报告关闭接口 '{}' 时出错 {}: {}", -(err->error), interface_name,
                                         strerror(-(err->error))));
            }
        }
        close(sock);

        if (!ack_success)
        {
            return unexpected(format("错误: 未能通过Netlink ACK确认接口 '{}' 的关闭。", interface_name));
        }
        return {};
    }
}
