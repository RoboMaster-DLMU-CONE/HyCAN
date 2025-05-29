#include "Interface/Interface.hpp"
#include "Interface/Logger.hpp"

namespace HyCAN
{
    Interface::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                        netlink(this->interface_name),
                                                        reaper(this->interface_name),
                                                        sender(this->interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Interface_{}", this->interface_name));
    }

    void Interface::up()
    {
        netlink.up();
        reaper.start();
    }

    void Interface::down()
    {
        netlink.down();
        reaper.stop();
    }
}
