#include "HyCAN/Interface/Interface.hpp"
using std::string, tl::unexpected;
using enum HyCAN::InterfaceErrorCode;

namespace HyCAN
{
    Interface::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                        netlink(this->interface_name),
                                                        reaper(this->interface_name),
                                                        sender(this->interface_name)
    {
    }

    tl::expected<void, InterfaceError> Interface::up()
    {
        return netlink.up().map_error([](const auto& e) { return InterfaceError{NetlinkError, e}; })
                      .and_then([&]
                      {
                          return reaper.start().map_error([](const auto& e) { return InterfaceError{ReaperError, e}; });
                      });
    }

    tl::expected<void, InterfaceError> Interface::down()
    {
        return netlink.down().map_error([](const auto& e) { return InterfaceError{NetlinkError, e}; })
                      .and_then([&]
                      {
                          return reaper.stop().map_error([](const auto& e) { return InterfaceError{ReaperError, e}; });
                      });
    }
}
