#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <xtr/logger.hpp>
#include "CanFrameConvertible.hpp"
#include "Netlink.hpp"
#include "Reaper.hpp"
#include "Sender.hpp"

using std::string;

namespace HyCAN
{
    class Interface
    {
    public:
        explicit Interface(const string& interface_name);
        Interface() = delete;
        void up();
        void down();

        template <CanFrameConvertiable T>
        void send(T frame) { sender.send(std::move(frame)); };

    private:
        std::string interface_name;
        Netlink netlink;
        Reaper reaper;
        Sender sender;
        sink s;
    };
}

#endif //INTERFACE_HPP
