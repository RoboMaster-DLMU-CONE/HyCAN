#ifndef VCAN_HPP
#define VCAN_HPP

#include <string>

#include "HyCAN/Util/Error.hpp"
#include "tl/expected.hpp"

namespace HyCAN
{
    tl::expected<void, Error> create_vcan_interface_if_not_exists(std::string_view interface_name) noexcept;
}
#endif //VCAN_HPP
