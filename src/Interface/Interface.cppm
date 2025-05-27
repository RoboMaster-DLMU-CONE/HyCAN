module;
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <format>
#include <exception>

#include <xtr/logger.hpp>
export module HyCAN.Interface;
import HyCAN.Interface.Logger;
import HyCAN.Interface.Netlink;
import HyCAN.Interface.Reaper;
using std::expected, std::unexpected, std::unique_ptr, std::string, std::string_view, std::format, std::exception;
using xtr::sink;

export namespace HyCAN
{
    template <InterfaceType type>
    class Interface
    {
    public:
        explicit Interface(std::string_view interface_name);
        Interface() = delete;
        void up();
        void down();

    private:
        std::string_view interface_name;
        Netlink<type> netlink;
        Reaper reaper;
        sink s;
    };

    template <InterfaceType type>
    Interface<type>::Interface(const std::string_view interface_name): interface_name(interface_name),
                                                                       netlink(interface_name), reaper(interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Interface_{}", interface_name));
    }

    template <InterfaceType type>
    void Interface<type>::up()
    {
        netlink.up();
        reaper.start();
    }

    template <InterfaceType type>
    void Interface<type>::down()
    {
        netlink.down();
        reaper.stop();
    }
}
