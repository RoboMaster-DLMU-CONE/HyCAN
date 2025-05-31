#include "HyCAN/Interface/Interface.hpp"
#include "HyCAN/Interface/Logger.hpp"

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

    expected<void, string> Interface::registerCallback(const size_t can_id, const function<void(can_frame&&)>& func)
    {
        auto result = reaper.registerFunc(can_id, func);
        if (!result)
        {
            XTR_LOGL(error, s, "{}", result.error());
        }
        return result;
    }
}
