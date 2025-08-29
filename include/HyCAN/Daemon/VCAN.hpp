#pragma once

#include <string_view>
#include <HyCAN/Util/Error.hpp>
#include <tl/expected.hpp>

namespace HyCAN
{
    /**
     * @brief Create VCAN interface if it doesn't exist
     * @param interface_name Name of the VCAN interface to create
     * @return Success or error information
     */
    tl::expected<void, Error> create_vcan_interface_if_not_exists(std::string_view interface_name) noexcept;
} // namespace HyCAN
