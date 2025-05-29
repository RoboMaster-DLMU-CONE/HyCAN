module;
#include <concepts>
#include <linux/can.h>
export module HyCAN.Interface.CanFrameConvertible;

export namespace HyCAN
{
    template <typename T>
    concept CanFrameConvertiable = requires(T a)
    {
        { static_cast<can_frame>(a) } -> std::same_as<can_frame>;
        { static_cast<T>(can_frame()) } -> std::same_as<T>;
    };
}
