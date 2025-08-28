#include "HyCAN/Interface/VCAN.hpp"
#include <format>
#include <cstdlib>
#include <cstring>

#include <net/if.h>


using tl::unexpected, std::format;

namespace HyCAN
{
    tl::expected<void, Error> create_vcan_interface_if_not_exists(const std::string_view interface_name) noexcept
    {
        if (if_nametoindex(interface_name.data()) != 0)
        {
            return {}; // already exist.
        }
        if (errno != ENODEV) // if_nametoindex failed for a reason other than "No such device"
        {
            return unexpected(Error{
                ErrorCode::VCANCheckingError, format("Error checking interface '{}' before creation: {}",
                                                     interface_name,
                                                     strerror(errno))
            });
        }

        // Creating VCAN interface using system command
        std::string command = format("ip link add {} type vcan", interface_name);
        
        if (const int result = std::system(command.c_str()); result != 0)
        {
            return unexpected(Error{
                ErrorCode::RtnlLinkAddError, format("Error: Cannot create interface: system() returned {}.", result)
            });
        }
        
        return {};
    }
}
