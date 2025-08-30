#include <iostream>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <format>

#include "HyCAN/Daemon/Message.hpp"
#include "HyCAN/Daemon/Daemon.hpp"
#include "HyCAN/Daemon/VCAN.hpp"
#include "../../include/HyCAN/Daemon/UnixSocket/UnixSocket.hpp"

namespace HyCAN
{
    Daemon::Daemon()
    {
        // Initialize netlink manager
        netlink_manager_ = std::make_unique<NetlinkManager>();
        if (netlink_manager_->initialize() < 0)
        {
            throw std::runtime_error("Failed to initialize netlink manager in daemon");
        }
        
        // Initialize main socket
        main_socket_ = std::make_unique<UnixSocket>("daemon", UnixSocket::SERVER);
        if (!main_socket_->initialize())
        {
            throw std::runtime_error("Failed to initialize main daemon socket");
        }
        
        // Start cleanup thread
        cleanup_thread_ = std::thread(&Daemon::session_cleanup_worker, this);
    }

    Daemon::~Daemon()
    {
        stop();
    }

    int Daemon::run()
    {
        if (geteuid() != 0)
        {
            std::cerr << "HyCAN daemon must run as root" << std::endl;
            return 1;
        }

        std::cout << "HyCAN Daemon started, listening on socket..." << std::endl;

        while (running_.load(std::memory_order_acquire))
        {
            try
            {
                // Accept incoming connections with timeout
                auto client_socket = main_socket_->accept(1000); // 1 second timeout
                if (!client_socket)
                {
                    // No incoming connection within timeout, continue
                    continue;
                }

                // Receive registration request
                ClientRegisterRequest register_request;
                const ssize_t bytes_received = client_socket->recv(&register_request, sizeof(register_request), 1000);
                
                if (bytes_received == sizeof(ClientRegisterRequest))
                {
                    handle_client_registration(register_request, std::move(client_socket));
                }
                else if (bytes_received != 0)
                {
                    std::cerr << "Received invalid request size: " << bytes_received
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

    void Daemon::handle_client_registration(const ClientRegisterRequest& request, const std::unique_ptr<UnixSocket>& registration_socket)
    {
        std::lock_guard lock(sessions_mutex_);
        
        std::cout << "Registering client with PID: " << request.client_pid << std::endl;
        
        // Check if client already exists
        if (client_sessions_.contains(request.client_pid))
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
                // Create dedicated socket for this client
                session->socket = std::make_unique<UnixSocket>(session->channel_name, UnixSocket::SERVER);
                
                if (!session->socket->initialize())
                {
                    std::cerr << "Failed to initialize client socket: " << session->channel_name << std::endl;
                    return;
                }
                
                // Store session first
                auto* session_ptr = session.get();
                client_sessions_[request.client_pid] = std::move(session);
                
                // Create worker thread for this client
                session_ptr->worker_thread = std::make_unique<std::thread>(&Daemon::client_session_worker, this, 
                                                                          session_ptr);
                
                std::cout << "Created dedicated socket: " << client_sessions_[request.client_pid]->channel_name << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to create client session: " << e.what() << std::endl;
                return;
            }
        }
        
        // Send response with channel name
        const ClientRegisterResponse response(0, client_sessions_[request.client_pid]->channel_name);
        
        if (registration_socket->send(&response, sizeof(response)) < 0)
        {
            std::cerr << "Failed to send registration response to client " << request.client_pid << std::endl;
        }
    }

    void Daemon::client_session_worker(ClientSession* session)
    {
        std::cout << "Starting worker thread for client " << session->client_pid << std::endl;
        
        while (session->running.load() && running_.load())
        {
            try
            {
                // Accept incoming connection from client with timeout
                const auto client_connection = session->socket->accept(1000); // 1 second timeout
                if (!client_connection)
                {
                    continue; // timeout, continue checking the running flags
                }
                
                // Receive request
                NetlinkRequest request;
                const ssize_t bytes_received = client_connection->recv(&request, sizeof(request), 1000);
                
                if (bytes_received == 0)
                {
                    continue; // Timeout, continue
                }
                
                if (bytes_received != sizeof(NetlinkRequest))
                {
                    std::cerr << "Client " << session->client_pid << " sent invalid request size: " 
                              << bytes_received << std::endl;
                    continue;
                }
                
                session->last_activity = std::chrono::steady_clock::now();
                
                std::cout << "Processing request from client " << session->client_pid 
                          << " for interface: " << request.interface_name << std::endl;
                
                // Process the request using netlink manager
                auto response = netlink_manager_->process_request(request);
                
                // Send response
                if (client_connection->send(&response, sizeof(response)) < 0)
                {
                    std::cerr << "Failed to send response to client " << session->client_pid << std::endl;
                }
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
        std::lock_guard lock(sessions_mutex_);
        
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

    bool Daemon::is_process_alive(const pid_t pid) const
    {
        return kill(pid, 0) == 0;
    }
}
