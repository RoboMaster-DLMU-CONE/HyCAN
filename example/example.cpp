#include <thread>
#include <chrono>
#include <iostream>
#include <linux/can.h>

#include "Interface/Interface.hpp"

using HyCAN::Interface;

int main()
{
    const function<void(can_frame&&)> callback = [](can_frame&& frame)
    {
        for (const auto i : frame.data)
        {
            std::cout<< i << " ";
        }
        std::cout<< "\n";
    };
    constexpr can_frame test_frame = {
        .can_id = 0x100,
        .len = 8,
        .data = {0,1,2,3,4,5,6,7},
    };

    Interface interface("vcan0");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    [[maybe_unused]] auto _ = interface.registerCallback(0x100, callback);
    interface.up();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < 5; ++i)
    {
        interface.send(test_frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::this_thread::sleep_for(std::chrono::seconds(4));
    interface.down();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
