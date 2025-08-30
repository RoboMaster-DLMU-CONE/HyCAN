#include <HyCAN/Interface/Netlink.hpp>
#include <HyCAN/Interface/NetlinkClient.hpp>

#include <format>
#include <memory>

using tl::unexpected;

namespace HyCAN
{
    // Netlink singleton implementation
    Netlink::Netlink() : main_channel_name_("HyCAN_Daemon")
    {
    }

    Netlink::~Netlink() 
    {
        // Explicitly reset the client before destruction
        client_.reset();
    }

    Netlink& Netlink::instance()
    {
        static Netlink instance;
        return instance;
    }

    tl::expected<void, Error> Netlink::ensure_initialized()
    {
        if (initialized_)
        {
            return {};
        }

        try
        {
            client_ = std::make_unique<NetlinkClient>();
            initialized_ = true;
            return {};
        }
        catch (const std::exception& e)
        {
            return unexpected(Error{
                ErrorCode::NetlinkBringUpError,
                std::format("Failed to initialize Netlink: {}", e.what())
            });
        }
    }

    tl::expected<void, Error> Netlink::set(const std::string_view interface_name, const bool up, const uint32_t bitrate)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->set_interface_state(interface_name, up, bitrate);
    }

    tl::expected<bool, Error> Netlink::exists(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->interface_exists(interface_name);
    }

    tl::expected<bool, Error> Netlink::is_up(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->interface_is_up(interface_name);
    }

    tl::expected<void, Error> Netlink::create_vcan(std::string_view interface_name)
    {
        auto init_result = ensure_initialized();
        if (!init_result)
        {
            return unexpected(init_result.error());
        }

        return client_->create_vcan_interface(interface_name);
    }
} // namespace HyCAN