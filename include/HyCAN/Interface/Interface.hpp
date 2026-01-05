#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include "CanFrameConvertible.hpp"
#include "Dispatcher.hpp"
#include "Sender.hpp"
#include <cstdint>
#include <set>
#include <string>

namespace HyCAN {
enum class InterfaceType { CAN, VCAN };

template <InterfaceType Type = InterfaceType::CAN> class Interface {
  public:
    explicit Interface(const std::string &interface_name);
    Interface() = delete;
    tl::expected<void, Error> up(uint32_t bitrate = 1000000);
    tl::expected<void, Error> down();
    tl::expected<bool, Error> exists();
    tl::expected<bool, Error> is_up();

    template <CanFrameConvertible T> tl::expected<void, Error> send(T frame) {
        return sender.send(frame);
    };

    template <typename T = can_frame, typename Func>
        requires(CanFrameConvertible<T> && std::invocable<Func, T>)
    tl::expected<void, Error> register_callback(const std::set<size_t> &can_ids,
                                                Func &&func) {
        return dispatcher.register_func<T>(can_ids, func);
    }
#ifdef HYCAN_LATENCY_TEST
    Dispatcher::LatencyStats get_reaper_latency_stats() const {
        return dispatcher.get_latency_stats();
    }
#endif

  private:
    std::string interface_name;
    Dispatcher dispatcher;
    Sender sender;
};

// Type aliases for common usage
using VCANInterface = Interface<InterfaceType::VCAN>;
using CANInterface = Interface<InterfaceType::CAN>;
} // namespace HyCAN

#endif // INTERFACE_HPP
