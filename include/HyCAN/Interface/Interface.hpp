#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <set>
#include <cstdint>
#include "CanFrameConvertible.hpp"
#include "IPCManager.hpp"
#include "Dispatcher.hpp"
#include "Sender.hpp"


namespace HyCAN
{
    enum class InterfaceType
    {
        CAN,
        VCAN
    };

    template <InterfaceType Type = InterfaceType::CAN>
    class Interface
    {
    public:
        explicit Interface(const std::string& interface_name);
        Interface() = delete;
        tl::expected<void, Error> up(uint32_t bitrate = 1000000);
        tl::expected<void, Error> down();
        tl::expected<bool, Error> exists();
        tl::expected<bool, Error> is_up();

        template <CanFrameConvertible T>
        tl::expected<void, Error> send(T frame)
        {
            return sender.send(std::move(frame));
        };

        template <CanFrameConvertible T = can_frame>
        tl::expected<void, Error> register_callback(const std::set<size_t>& can_ids,
                                                    const std::function<void(T&&)>& func)
        {
            return reaper.tryRegisterFunc<T>(can_ids, func);
        }
#ifdef HYCAN_LATENCY_TEST
        Dispatcher::LatencyStats get_reaper_latency_stats() const
        {
            return reaper.get_latency_stats();
        }
#endif

    private:
        std::string interface_name;
        Dispatcher reaper;
        Sender sender;
    };

    // Type aliases for common usage
    using VCANInterface = Interface<InterfaceType::VCAN>;
    using CANInterface = Interface<InterfaceType::CAN>;
}

#endif //INTERFACE_HPP
