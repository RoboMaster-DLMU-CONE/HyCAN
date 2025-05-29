module;
#include <memory>
#include <string>
#include <format>

#include <xtr/logger.hpp>
export module HyCAN.Interface;
import HyCAN.Interface.CanFrameConvertible;
import HyCAN.Interface.Logger;
import HyCAN.Interface.Reaper;
import HyCAN.Interface.Sender;
export import HyCAN.Interface.Netlink;
using std::unique_ptr, std::string, std::format, std::exception, std::move;
using xtr::sink;
export namespace HyCAN
{
    template <InterfaceType type>
    class Interface
    {
    public:
        explicit Interface(const string& interface_name);
        Interface() = delete;
        void up();
        void down();
        template <CanFrameConvertiable T>
        void send(T frame);

    private:
        std::string interface_name;
        Netlink<type> netlink;
        Reaper reaper;
        Sender sender;
        sink s;
    };

    template <InterfaceType type>
    Interface<type>::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                              netlink(this->interface_name),
                                                              reaper(this->interface_name),
                                                              sender(this->interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Interface_{}", this->interface_name));
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

    template <InterfaceType type>
    template <CanFrameConvertiable T>
    void Interface<type>::send(T frame)
    {
        sender.send(move(frame));
    }
}
