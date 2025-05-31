#ifndef CANFRAMECONVERTIBLE_HPP
#define CANFRAMECONVERTIBLE_HPP

#include <concepts>
#include <linux/can.h>

namespace HyCAN
{
    template <typename T>
    concept CanFrameConvertible = requires(T a)
    {
        { static_cast<can_frame>(a) } -> std::same_as<can_frame>;
        { static_cast<T>(can_frame()) } -> std::same_as<T>;
    };
}

#endif //CANFRAMECONVERTIBLE_HPP
