#include "Interface/Sender.hpp"
#include "Interface/Logger.hpp"

namespace HyCAN
{
    Sender::Sender(const string_view interface_name): socket(interface_name), interface_name(interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Sender_{}", interface_name));
    }
}
