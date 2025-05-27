module;
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/rtnetlink.h>
#include <xtr/logger.hpp>
export module HyCAN.Interface.Netlink;
import HyCAN.Interface.Logger;
import :VCAN;

using std::format, std::string_view, std::string;
using xtr::logger, xtr::sink;

export namespace HyCAN
{
    enum class InterfaceType
    {
        Virtual,
        Physics
    };

    template <InterfaceType type>
    class Netlink
    {
    public:
        explicit Netlink(string_view interface_name);
        Netlink() = delete;
        void up();
        void down();

    private:
        string_view interface_name;
        int sock{};

        struct NetlinkIfInfoRequest
        {
            nlmsghdr nlh;
            ifinfomsg ifm;
        };

        NetlinkIfInfoRequest req{};

        sink s;
    };

    template <InterfaceType type>
    Netlink<type>::Netlink(string_view interface_name): interface_name(interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Netlink_{}", interface_name));
        if constexpr (type == InterfaceType::Virtual)
        {
            if (auto result = create_vcan_interface_if_not_exists(interface_name); !result)
            {
                XTR_LOGL(fatal, s, "Error during vcan creation: {}", result.error());
            }
        }
    }

    template <InterfaceType type>
    void Netlink<type>::up()
    {
        const unsigned int if_index = if_nametoindex(interface_name.data());
        if (if_index == 0)
        {
            XTR_LOGL(fatal, s, "Error: Cannot find interface '{}': {}", interface_name, strerror(errno));
        }
        sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (sock < 0)
        {
            XTR_LOGL(fatal, s, "Error: Cannot open Netlink socket for interface '{}': {}", interface_name,
                     strerror(errno));
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
            XTR_LOGL(fatal, s, "Error: Failed to send Netlink message for interface '{}': {}",
                     interface_name,
                     strerror(errno));
        }
        close(sock);
    }

    template <InterfaceType type>
    void Netlink<type>::down()
    {
        const unsigned int if_index = if_nametoindex(interface_name.data());
        if (if_index == 0)
        {
            XTR_LOGL(fatal, s,
                     "Error: Cannot find interface '{}' to bring down: {}",
                     interface_name, strerror(errno));
        }

        sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (sock < 0)
        {
            XTR_LOGL(fatal, s,
                     "Error: Cannot open Netlink socket for interface '{}': {}",
                     interface_name, strerror(errno));
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
            XTR_LOGL(fatal, s,
                     "Error: Failed to send Netlink message to bring down interface '{}': {}",
                     interface_name, strerror(send_errno));
        }

        // 等待ACK
        char recv_buf[NLMSG_ALIGN(NLMSG_HDRLEN) + NLMSG_ALIGN(sizeof(struct nlmsgerr)) + 256];
        ssize_t len = recv(sock, recv_buf, sizeof(recv_buf), 0);
        if (len < 0)
        {
            const int recv_errno = errno;
            close(sock);
            XTR_LOGL(fatal, s,
                     "Error: Failed to receive ACK for bringing down interface '{}': {}",
                     interface_name, strerror(recv_errno));
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
                XTR_LOGL(fatal, s,
                         "Error: Netlink ACK reported error {} while bringing down interface '{}': {}",
                         -(err->error), interface_name, strerror(-(err->error)));
            }
        }
        close(sock);

        if (!ack_success)
        {
            XTR_LOGL(fatal, s,
                     "Error: Did not receive Netlink ACK confirming interface '{}' down.",
                     interface_name);
        }
    }
}
