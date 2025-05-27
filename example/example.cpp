import HyCAN.Interface;
#include <thread>
#include <chrono>

using HyCAN::Interface;
using enum HyCAN::InterfaceType;

int main()
{
    Interface<Virtual> interface("vcan0");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    interface.up();
    std::this_thread::sleep_for(std::chrono::seconds(4));
    interface.down();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
