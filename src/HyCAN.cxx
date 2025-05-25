module;
#include <expected>
#include <iostream>
export module HyCAN;
import :Interface;


export void init()
{
    std::cout << "Hello HyCAN" << std::endl;
    if (const auto result = set_interface_up("vcan0"); !result)
    {
        std::cerr << "Failed to set up interface. Details: " << result.error() <<
            std::endl;
    }
}
