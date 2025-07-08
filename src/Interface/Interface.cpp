#include "HyCAN/Interface/Interface.hpp"
using std::string, tl::unexpected;

namespace HyCAN
{
    Interface::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                        netlink(this->interface_name),
                                                        reaper(this->interface_name),
                                                        sender(this->interface_name)
    {
    }

    tl::expected<void, std::string> Interface::up()
    {
        if (const auto result = netlink.up(); !result)
        {
            return unexpected(result.error());
        }
        return reaper.start();
    }

    tl::expected<void, std::string> Interface::down()
    {
        if (const auto result = netlink.down(); !result)
        {
            return unexpected(result.error());
        }
        return reaper.stop();
    }
}
