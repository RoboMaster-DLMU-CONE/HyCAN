import HyCAN.Interface;
#include <thread>
#include <chrono>
#include <linux/can.h>

using HyCAN::Interface;

int main()
{
    Interface interface("vcan0");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    interface.up();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    interface.send(can_frame{});
    std::this_thread::sleep_for(std::chrono::seconds(4));
    interface.down();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
