#include "hycan/Interface/Interface.hpp"
#include "hycan/Interface/Logger.hpp"

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

    void Interface::tryRegisterCallback(const set<size_t>& can_ids, const function<void(can_frame&&)>& func)
    {
        reaper.tryRegisterFunc(can_ids, func);
    }
}
