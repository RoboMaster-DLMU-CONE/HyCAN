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
    class Interface
    {
    public:
        explicit Interface(const std::string& interface_name);
        Interface() = delete;
        virtual ~Interface() = default;
        
        virtual tl::expected<void, Error> up();
        virtual tl::expected<void, Error> down();
        virtual tl::expected<bool, Error> is_up();

        template <CanFrameConvertible T>
        tl::expected<void, Error> send(T frame)
        {
            return sender.send(std::move(frame));
        };

        template <CanFrameConvertible T = can_frame>
        tl::expected<void, Error> tryRegisterCallback(const std::set<size_t>& can_ids,
                                                      const std::function<void(T&&)>& func)
        {
            return reaper.tryRegisterFunc<T>(can_ids, func);
        }
#ifdef HYCAN_LATENCY_TEST
        Reaper::LatencyStats get_reaper_latency_stats() const
        {
            return reaper.get_latency_stats();
        }
#endif

    protected:
        std::string interface_name;
        Netlink netlink;
        Reaper reaper;
        Sender sender;
    };
}

#endif //INTERFACE_HPP
