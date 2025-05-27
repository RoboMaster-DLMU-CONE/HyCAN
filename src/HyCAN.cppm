module;
#include <xtr/logger.hpp>
export module HyCAN;
import HyCAN.Interface;

export namespace HyCAN
{
    void init()
    {
        xtr::logger log;
        xtr::sink s = log.get_sink("HyCAN Init");
        XTR_LOGL(info, s, "Hello HyCAN!");
        auto interface = Interface<Virtual>("vcan0");
        if (const auto result = interface.up(); !result)
        {
            XTR_LOGL(error, s, "Failed to set up interface. Details: {}", result.error());
        }
        if (const auto result = interface.down(); !result)
        {
            XTR_LOGL(error, s, "Failed to set down interface. Details: {}", result.error());
        }
    }
}
