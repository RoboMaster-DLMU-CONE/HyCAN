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
using std::unique_ptr, std::string, std::format, std::exception;
using xtr::sink;
export namespace HyCAN
{
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
        Netlink netlink;
        Reaper reaper;
        Sender sender;
        sink s;
    };

    Interface::Interface(const string& interface_name): interface_name(string(interface_name)),
                                                        netlink(this->interface_name),
                                                        reaper(this->interface_name),
                                                        sender(this->interface_name)
    {
        s = interface_logger.get_sink(format("HyCAN Interface_{}", this->interface_name));
    }

    void Interface::up()
    {
        netlink.up();
        reaper.start();
    }

    void Interface::down()
    {
        netlink.down();
        reaper.stop();
    }

    template <CanFrameConvertiable T>
    void Interface::send(T frame)
    {
        sender.send(std::move(frame));
    }
}
