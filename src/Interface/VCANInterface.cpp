#include "HyCAN/Interface/VCANInterface.hpp"

namespace HyCAN
{
    VCANInterface::VCANInterface(const std::string& interface_name) 
        : Interface(interface_name)
    {
        // For VCAN interfaces, we don't validate on construction
        // Instead, we create them on-demand when up() is called
    }

    tl::expected<void, Error> VCANInterface::up()
    {
        // For VCAN interfaces, the existing Netlink mechanism handles VCAN creation
        // when the interface name starts with "vcan"
        return Interface::up(); // Call the base class implementation
    }
}