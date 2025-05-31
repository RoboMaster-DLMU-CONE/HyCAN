#ifndef VCAN_HPP
#define VCAN_HPP


#include <xtr/logger.hpp>

using std::string, std::string_view, std::format, xtr::sink;

namespace HyCAN
{
    void create_vcan_interface_if_not_exists(string_view interface_name, sink& s);
}
#endif //VCAN_HPP
