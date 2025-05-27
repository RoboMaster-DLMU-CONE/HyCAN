module;
#include <xtr/logger.hpp>
export module HyCAN;
import HyCAN.Interface;
import HyCAN.Interface.Netlink;

export namespace HyCAN
{
    void init()
    {
        xtr::logger log;
        xtr::sink s = log.get_sink("HyCAN Init");
        XTR_LOGL(info, s, "Hello HyCAN!");
        auto interface = Interface<InterfaceType::Virtual>("vcan0");
        interface.up();
        std::this_thread::sleep_for(std::chrono::duration(std::chrono::seconds(1)));
        interface.down();
    }
}
