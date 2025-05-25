module;
#include <iostream>
export module HyCAN;
import HyCAN.Interface;

export void init()
{
    std::cout << "Hello HyCAN" << std::endl;
    if (const auto result = set_interface_up<CANInterfaceType::Virtual>("vcan0"); !result)
    {
        std::cerr << "Failed to set up interface. Details: " << result.error() <<
            std::endl;
    }
    if (const auto result = set_interface_down("vcan0"); !result)
    {
        std::cerr << "Failed to set down interface. Details: " << result.error() <<
            std::endl;
    }
}
