#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <set>
#include "CanFrameConvertible.hpp"
#include "Netlink.hpp"
#include "Reaper.hpp"
#include "Sender.hpp"


namespace HyCAN
{
    enum class InterfaceErrorCode
    {
        NetlinkError,
        ReaperError,
        SenderError,
        RegisterFuncError,
    };

    struct InterfaceError
    {
        InterfaceErrorCode code;
        std::string message;

        InterfaceError(const InterfaceErrorCode c, const std::string& msg): code(c), message(std::move(msg))
        {
        }
    };

    class Interface
    {
    public:
        explicit Interface(const std::string& interface_name);
        Interface() = delete;
        tl::expected<void, InterfaceError> up();
        tl::expected<void, InterfaceError> down();

        template <CanFrameConvertible T>
        tl::expected<void, InterfaceError> send(T frame)
        {
            return sender.send(std::move(frame)).map_error([](const auto& e)
            {
                return InterfaceError{InterfaceErrorCode::SenderError, e};
            });
        };

        template <CanFrameConvertible T = can_frame>
        tl::expected<void, InterfaceError> tryRegisterCallback(const std::set<size_t>& can_ids,
                                                               const std::function<void(T&&)>& func)
        {
            return reaper.tryRegisterFunc<T>(can_ids, func).map_error([](const auto& e)
            {
                return InterfaceError{InterfaceErrorCode::RegisterFuncError, e};
            });
        }
#ifdef HYCAN_LATENCY_TEST
        Reaper::LatencyStats get_reaper_latency_stats() const
        {
            return reaper.get_latency_stats();
        }
#endif

    private:
        std::string interface_name;
        Netlink netlink;
        Reaper reaper;
        Sender sender;
    };
}

#endif //INTERFACE_HPP
