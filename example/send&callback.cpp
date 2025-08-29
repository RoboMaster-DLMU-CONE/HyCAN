#include <thread>
#include <chrono>
#include <iostream>
#include <linux/can.h>

#include "HyCAN/Interface/VCANInterface.hpp"

using HyCAN::VCANInterface;

int main()
{
    const std::function callback = [](can_frame&& frame)
    {
        std::cout << "CAN ID: 0x" << std::hex << frame.can_id
            << " DLC: " << std::dec << static_cast<int>(frame.len)
            << " Data: ";
        for (int i = 0; i < frame.len; ++i)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(frame.data[i]) <<
                " ";
        }
        std::cout << std::dec << std::endl;
    };
    constexpr can_frame test_frame = {
        .can_id = 0x100,
        .len = 8,
        .data = {0, 1, 2, 3, 4, 5, 6, 7},
    };

    VCANInterface interface("vcan0");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    (void)interface.tryRegisterCallback({0x100}, callback)
                   .and_then([&] { return interface.up(); })
                   .map_error([&](const auto& err) { std::cerr << err.message << std::endl; });
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < 5; ++i)
    {
        (void)interface.send(test_frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::this_thread::sleep_for(std::chrono::seconds(4));
    (void)interface.down();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
