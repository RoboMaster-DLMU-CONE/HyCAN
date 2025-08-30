#include <iostream>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <format>
#include <net/if.h>
#include <sys/ioctl.h>
#include <cstring>

#include <netlink/cache.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>

#include "libipc/ipc.h"
#include "HyCAN/Daemon/Message.hpp"
#include "HyCAN/Daemon/Daemon.hpp"
#include "HyCAN/Daemon/VCAN.hpp"

namespace HyCAN
{
    Daemon::Daemon()
    {
        if (init_netlink() < 0)
        {
            throw std::runtime_error("Failed to initialize netlink in daemon");
        }
        
        // Start cleanup thread
        cleanup_thread_ = std::thread(&Daemon::session_cleanup_worker, this);
    }

    Daemon::~Daemon()
    {
        stop();
        cleanup_netlink();
    }

    int Daemon::run()
    {
        if (geteuid() != 0)
        {
            std::cerr << "HyCAN daemon must run as root" << std::endl;
            return 1;
        }

        std::cout << "HyCAN Daemon started, listening on channel 'HyCAN_Daemon'..." << std::endl;

        while (running_.load(std::memory_order_acquire))
        {
            try
            {
                // Wait for client requests with 1 second timeout
                auto received = main_channel_.recv(1000);

                if (received.empty())
                {
                    continue; // Timeout, continue checking stop flag
                }

                // Handle client registration requests
                if (received.size() == sizeof(ClientRegisterRequest))
                {
                    const auto* register_request = static_cast<const ClientRegisterRequest*>(received.data());
                    handle_client_registration(*register_request);
                }
                else
                {
                    std::cerr << "Received invalid request size: " << received.size() 
                              << " (expected: " << sizeof(ClientRegisterRequest) << ")" << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception in daemon main loop: " << e.what() << std::endl;
            }
        }

        cleanup_sessions();
        std::cout << "HyCAN Daemon stopped." << std::endl;
        return 0;
    }

    void Daemon::stop()
    {
        running_.store(false, std::memory_order_release);
        
        // Join cleanup thread
        if (cleanup_thread_.joinable())
        {
            cleanup_thread_.join();
        }
    }

    std::string Daemon::generate_client_channel_name(pid_t client_pid)
    {
        return std::format("HyCAN_Client_{}", client_pid);
    }

    void Daemon::handle_client_registration(const ClientRegisterRequest& request)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        
        std::cout << "Registering client with PID: " << request.client_pid << std::endl;
        
        // Check if client already exists
        if (client_sessions_.find(request.client_pid) != client_sessions_.end())
        {
            std::cout << "Client " << request.client_pid << " already registered, updating..." << std::endl;
            // Update last activity time
            client_sessions_[request.client_pid]->last_activity = std::chrono::steady_clock::now();
        }
        else
        {
            // Create new client session
            auto session = std::make_unique<ClientSession>(request.client_pid, 
                                                         generate_client_channel_name(request.client_pid));
            
            try
            {
                // Create dedicated channel for this client
                session->channel = std::make_unique<ipc::channel>(session->channel_name.c_str(), ipc::receiver);
                
                // Store session first
                auto* session_ptr = session.get();
                client_sessions_[request.client_pid] = std::move(session);
                
                // Create worker thread for this client
                session_ptr->worker_thread = std::make_unique<std::thread>(&Daemon::client_session_worker, this, 
                                                                          session_ptr);
                
                std::cout << "Created dedicated channel: " << client_sessions_[request.client_pid]->channel_name << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to create client session: " << e.what() << std::endl;
            }
        }
        
        // Send response with channel name
        ClientRegisterResponse response(0, client_sessions_[request.client_pid]->channel_name);
        auto response_data = ipc::buff_t{reinterpret_cast<ipc::byte_t*>(&response), sizeof(response)};
        
        try
        {
            main_channel_.send(response_data);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to send registration response: " << e.what() << std::endl;
        }
    }

    void Daemon::client_session_worker(ClientSession* session)
    {
        std::cout << "Starting worker thread for client " << session->client_pid << std::endl;
        
        while (session->running.load() && running_.load())
        {
            try
            {
                auto received = session->channel->recv(1000); // 1 second timeout
                
                if (received.empty())
                {
                    continue; // Timeout, continue
                }
                
                session->last_activity = std::chrono::steady_clock::now();
                
                if (received.size() != sizeof(NetlinkRequest))
                {
                    std::cerr << "Client " << session->client_pid << " sent invalid request size: " 
                              << received.size() << std::endl;
                    continue;
                }
                
                const auto* request = static_cast<const NetlinkRequest*>(received.data());
                
                std::cout << "Processing request from client " << session->client_pid 
                          << " for interface: " << request->interface_name << std::endl;
                
                // Process the request
                auto response = process_request(*request);
                
                // Send response
                auto response_data = ipc::buff_t{reinterpret_cast<ipc::byte_t*>(&response), sizeof(response)};
                session->channel->send(response_data);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception in client worker " << session->client_pid << ": " << e.what() << std::endl;
            }
        }
        
        std::cout << "Worker thread for client " << session->client_pid << " stopped" << std::endl;
    }

    void Daemon::session_cleanup_worker()
    {
        constexpr auto cleanup_interval = std::chrono::seconds(30);
        constexpr auto session_timeout = std::chrono::minutes(5);
        
        while (running_.load())
        {
            std::this_thread::sleep_for(cleanup_interval);
            
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            auto now = std::chrono::steady_clock::now();
            
            for (auto it = client_sessions_.begin(); it != client_sessions_.end();)
            {
                auto& session = it->second;
                
                // Check if process is still alive and if session has timed out
                if (!is_process_alive(session->client_pid) || 
                    (now - session->last_activity) > session_timeout)
                {
                    std::cout << "Cleaning up session for client " << session->client_pid << std::endl;
                    
                    session->running.store(false);
                    if (session->worker_thread && session->worker_thread->joinable())
                    {
                        session->worker_thread->join();
                    }
                    
                    it = client_sessions_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    void Daemon::cleanup_sessions()
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        
        for (auto& [pid, session] : client_sessions_)
        {
            session->running.store(false);
            if (session->worker_thread && session->worker_thread->joinable())
            {
                session->worker_thread->join();
            }
        }
        
        client_sessions_.clear();
    }

    bool Daemon::is_process_alive(pid_t pid) const
    {
        return kill(pid, 0) == 0;
    }

    int Daemon::init_netlink()
    {
        nl_socket_ = nl_socket_alloc();
        if (!nl_socket_)
        {
            std::cerr << "Failed to allocate netlink socket" << std::endl;
            return -1;
        }

        if (nl_connect(nl_socket_, NETLINK_ROUTE) < 0)
        {
            std::cerr << "Failed to connect to netlink" << std::endl;
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
            return -1;
        }

        if (rtnl_link_alloc_cache(nl_socket_, AF_UNSPEC, &link_cache_) < 0)
        {
            std::cerr << "Failed to allocate link cache" << std::endl;
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
            return -1;
        }

        return 0;
    }

    void Daemon::cleanup_netlink()
    {
        if (link_cache_)
        {
            nl_cache_free(link_cache_);
            link_cache_ = nullptr;
        }
        if (nl_socket_)
        {
            nl_socket_free(nl_socket_);
            nl_socket_ = nullptr;
        }
    }

    NetlinkResponse Daemon::check_interface_exists(std::string_view interface_name) const
    {
        // Refresh cache to get latest state
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, false, false);
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        bool exists = (link != nullptr);
        
        if (link)
        {
            rtnl_link_put(link);
        }
        
        return NetlinkResponse(0, exists, false);
    }

    NetlinkResponse Daemon::check_interface_is_up(std::string_view interface_name) const
    {
        // Refresh cache to get latest state
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, false, false);
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        bool exists = (link != nullptr);
        bool is_up = false;
        
        if (link)
        {
            is_up = (rtnl_link_get_flags(link) & IFF_UP) != 0;
            rtnl_link_put(link);
        }
        
        return NetlinkResponse(0, exists, is_up);
    }

    NetlinkResponse Daemon::set_interface_state_libnl(std::string_view interface_name, const bool up) const
    {
        // 刷新缓存以获取最新状态
        if (nl_cache_refill(nl_socket_, link_cache_) < 0)
        {
            return NetlinkResponse(-1, "Failed to refresh link cache");
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        if (!link)
        {
            return NetlinkResponse(-1, std::format("Interface {} not found", interface_name));
        }

        rtnl_link* change = rtnl_link_alloc();
        if (!change)
        {
            rtnl_link_put(link);
            return NetlinkResponse(-1, "Failed to allocate change link object");
        }

        // 设置接口状态
        if (up)
        {
            rtnl_link_set_flags(change, IFF_UP);
        }
        else
        {
            rtnl_link_unset_flags(change, IFF_UP);
        }

        const int result = rtnl_link_change(nl_socket_, link, change, 0);

        rtnl_link_put(change);
        rtnl_link_put(link);

        if (result < 0)
        {
            return NetlinkResponse(result, std::format("rtnl_link_change failed: {}", nl_geterror(result)));
        }

        // 再次刷新缓存以反映更改
        nl_cache_refill(nl_socket_, link_cache_);

        return NetlinkResponse(0, "Success");
    }

    NetlinkResponse Daemon::set_can_bitrate_libnl(std::string_view interface_name, const uint32_t bitrate) const
    {
        if (!interface_name.starts_with("can"))
        {
            return NetlinkResponse(0, "Not a CAN interface, skipping bitrate setting");
        }

        rtnl_link* link = rtnl_link_get_by_name(link_cache_, std::string(interface_name).c_str());
        if (!link)
        {
            return NetlinkResponse(-1, std::format("CAN Interface {} not found", interface_name));
        }

        rtnl_link* change = rtnl_link_alloc();
        if (!change)
        {
            rtnl_link_put(link);
            return NetlinkResponse(-1, "Failed to allocate change link object for CAN");
        }

        // 设置 CAN 比特率
        if (rtnl_link_is_can(link))
        {
            rtnl_link_can_set_bitrate(change, bitrate);
            const int result = rtnl_link_change(nl_socket_, link, change, 0);

            rtnl_link_put(change);
            rtnl_link_put(link);

            if (result < 0)
            {
                return NetlinkResponse(result, std::format("Failed to set CAN bitrate: {}", nl_geterror(result)));
            }

            return NetlinkResponse(0, "CAN bitrate set successfully");
        }
        else
        {
            rtnl_link_put(change);
            rtnl_link_put(link);
            return NetlinkResponse(-1, "Interface is not a CAN interface");
        }
    }

    NetlinkResponse Daemon::process_request(const NetlinkRequest& request) const
    {
        switch (request.operation)
        {
            case RequestType::INTERFACE_EXISTS:
                return check_interface_exists(request.interface_name);
            
            case RequestType::INTERFACE_IS_UP:
                return check_interface_is_up(request.interface_name);
            
            case RequestType::CREATE_VCAN_INTERFACE:
            {
                auto vcan_result = create_vcan_interface_if_not_exists(request.interface_name);
                if (!vcan_result)
                {
                    return NetlinkResponse(-1, std::format("Failed to create VCAN interface {}: {}",
                                                           request.interface_name, vcan_result.error().message));
                }
                return NetlinkResponse(0, "VCAN interface created successfully");
            }
            
            case RequestType::SET_INTERFACE_STATE:
            default:
                // Create VCAN interface if needed
                if (request.create_vcan_if_needed)
                {
                    auto vcan_result = create_vcan_interface_if_not_exists(request.interface_name);
                    if (!vcan_result)
                    {
                        return NetlinkResponse(-1, std::format("Failed to create VCAN interface {}: {}",
                                                               request.interface_name, vcan_result.error().message));
                    }
                }

                // 如果需要设置比特率（仅对 CAN 接口）
                if (request.up && request.set_bitrate)
                {
                    const auto bitrate_result = set_can_bitrate_libnl(request.interface_name, request.bitrate);
                    if (bitrate_result.result != 0)
                    {
                        return bitrate_result;
                    }
                }

                // 设置接口状态
                return set_interface_state_libnl(request.interface_name, request.up);
        }
    }
}
