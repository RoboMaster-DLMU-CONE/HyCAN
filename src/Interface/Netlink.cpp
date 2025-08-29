#include "HyCAN/Interface/Netlink.hpp"
#include "HyCAN/Interface/Daemon.hpp"
#include "HyCAN/Config.hpp"

#include <stdexcept>

using std::string_view;

namespace HyCAN
{
    Netlink::Netlink(const string_view interface_name) : interface_name(interface_name)
    {
        daemon_client_ = std::make_unique<DaemonClient>(Config::get_daemon_socket_path());
        
        // Create VCAN interface if it doesn't exist
        (void)daemon_client_->create_vcan_interface(interface_name).map_error([](const auto& e)
        {
            throw std::runtime_error(e.message);
        });
    }

    Netlink::~Netlink() = default;

    tl::expected<void, Error> Netlink::up() noexcept
    {
        return daemon_client_->set_interface_up(interface_name);
    }

    tl::expected<void, Error> Netlink::down() noexcept
    {
        return daemon_client_->set_interface_down(interface_name);
    }
}
