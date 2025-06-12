#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <set>
#include <expected>
#include "CanFrameConvertible.hpp"
#include "Netlink.hpp"
#include "Reaper.hpp"
#include "Sender.hpp"

using Result = std::expected<void, std::string>;

namespace HyCAN
{
    class Interface
    {
    public:
        explicit Interface(const std::string& interface_name);
        Interface() = delete;
        Result up();
        Result down();

        template <CanFrameConvertible T>
        Result send(T frame) { return sender.send(std::move(frame)); };

        template <CanFrameConvertible T = can_frame>
        Result tryRegisterCallback(const std::set<size_t>& can_ids, const std::function<void(T&&)>& func)
        {
            return reaper.tryRegisterFunc<T>(can_ids, func);
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
