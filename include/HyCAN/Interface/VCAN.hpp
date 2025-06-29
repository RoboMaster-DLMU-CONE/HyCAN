#ifndef VCAN_HPP
#define VCAN_HPP

#include <string>
#include <format>
#include <expected>

namespace HyCAN
{
    using Result = std::expected<void, std::string>;
    Result create_vcan_interface_if_not_exists(std::string_view interface_name) noexcept;
}
#endif //VCAN_HPP
