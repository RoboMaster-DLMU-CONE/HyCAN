#include "HyCAN/Interface/Interface.hpp"
using std::string, std::unexpected;

namespace HyCAN
{
    Interface::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                        netlink(this->interface_name),
                                                        reaper(this->interface_name),
                                                        sender(this->interface_name)
    {
    }

    Result Interface::up()
    {
        if (const auto result = netlink.up(); !result)
        {
            return unexpected(result.error());
        }
        return reaper.start();
    }

    Result Interface::down()
    {
        if (const auto result = netlink.down(); !result)
        {
            return unexpected(result.error());
        }
        return reaper.stop();
    }
}
