#include "HyCAN/Interface/Interface.hpp"
using std::string, tl::unexpected;

namespace HyCAN
{
    Interface::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                        netlink(this->interface_name),
                                                        reaper(this->interface_name),
                                                        sender(this->interface_name)
    {
    }

    tl::expected<void, Error> Interface::up()
    {
        return netlink.up()
                      .and_then([&] { return reaper.start(); });
    }

    tl::expected<void, Error> Interface::down()
    {
        return netlink.down()
                      .and_then([&]
                      {
                          return reaper.stop();
                      });
    }

    tl::expected<bool, Error> Interface::is_up()
    {
        return netlink.is_up();
    }
}
