#include "HyCAN/Interface/Interface.hpp"
#include "HyCAN/Interface/Netlink.hpp"
#include <format>
using std::string, tl::unexpected;

namespace HyCAN
{
    template <InterfaceType Type>
    Interface<Type>::Interface(const string& interface_name) : interface_name(string(interface_name)),
                                                               reaper(this->interface_name),
                                                               sender(this->interface_name)
    {
    }

    template <InterfaceType Type>
    tl::expected<void, Error> Interface<Type>::up(const uint32_t bitrate)
    {
        if constexpr (Type == InterfaceType::VCAN)
        {
            // For VCAN, create the interface if it doesn't exist
            auto exists_result = Netlink::instance().exists(interface_name);
            if (!exists_result)
            {
                return unexpected(exists_result.error());
            }

            if (!exists_result.value())
            {
                // Create VCAN interface
                auto create_result = Netlink::instance().create_vcan(interface_name);
                if (!create_result)
                {
                    return unexpected(create_result.error());
                }
            }
        }
        else if constexpr (Type == InterfaceType::CAN)
        {
            // For CAN, ensure the interface exists (error if not found)
            auto exists_result = Netlink::instance().exists(interface_name);
            if (!exists_result)
            {
                return unexpected(exists_result.error());
            }

            if (!exists_result.value())
            {
                return unexpected(Error{
                    ErrorCode::NetlinkBringUpError,
                    std::format("CAN interface '{}' not found", interface_name)
                });
            }
        }

        return Netlink::instance().set(interface_name, true, bitrate)
                                  .and_then([&] { return reaper.start(); });
    }

    template <InterfaceType Type>
    tl::expected<void, Error> Interface<Type>::down()
    {
        return Netlink::instance().set(interface_name, false)
                                  .and_then([&]
                                   {
                                       return reaper.stop();
                                   });
    }

    template <InterfaceType Type>
    tl::expected<bool, Error> Interface<Type>::exists()
    {
        return Netlink::instance().exists(interface_name);
    }

    template <InterfaceType Type>
    tl::expected<bool, Error> Interface<Type>::is_up()
    {
        return Netlink::instance().is_up(interface_name);
    }

    // Explicit template instantiations
    template class Interface<InterfaceType::VCAN>;
    template class Interface<InterfaceType::CAN>;
}
