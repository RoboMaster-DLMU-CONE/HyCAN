#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <set>
#include <xtr/logger.hpp>
#include "CanFrameConvertible.hpp"
#include "Netlink.hpp"
#include "Reaper.hpp"
#include "Sender.hpp"

using std::string, std::set;

namespace HyCAN
{
    class Interface
    {
    public:
        explicit Interface(const string& interface_name);
        Interface() = delete;
        void up();
        void down();

        template <CanFrameConvertible T>
        void send(T frame) { sender.send(std::move(frame)); };

        void tryRegisterCallback(const set<size_t>& can_ids, const function<void(can_frame&&)>& func);
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
        sink s;
    };
}

#endif //INTERFACE_HPP
