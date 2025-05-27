import HyCAN.Interface;
import HyCAN.Interface.Netlink;
#include <thread>
#include <chrono>

int main()
{
    HyCAN::Interface<HyCAN::InterfaceType::Virtual> interface("vcan0");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
