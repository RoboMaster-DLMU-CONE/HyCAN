#include "HyCAN/Interface/Sender.hpp"

namespace HyCAN
{
    Sender::Sender(const std::string_view interface_name): socket(interface_name), interface_name(interface_name)
    {
    }
}
