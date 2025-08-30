#include <HyCAN/Daemon/UnixSocket/UnixSocket.hpp>
#include <iostream>
#include <sys/select.h>
#include <algorithm>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>

namespace HyCAN
{
    UnixSocket::UnixSocket(const std::string& path, const Mode mode)
        : socket_path_("/run/hycan_" + path), mode_(mode)
    {
    }

    UnixSocket::UnixSocket(const int fd, const std::string& path)
        : socket_fd_(fd), socket_path_(path), mode_(CLIENT), connected_(true)
    {
    }

    UnixSocket::~UnixSocket()
    {
        close();
    }

    UnixSocket::UnixSocket(UnixSocket&& other) noexcept
        : socket_fd_(other.socket_fd_)
        , socket_path_(std::move(other.socket_path_))
        , mode_(other.mode_)
        , connected_(other.connected_)
    {
        other.socket_fd_ = -1;
        other.connected_ = false;
    }

    UnixSocket& UnixSocket::operator=(UnixSocket&& other) noexcept
    {
        if (this != &other)
        {
            close();
            socket_fd_ = other.socket_fd_;
            socket_path_ = std::move(other.socket_path_);
            mode_ = other.mode_;
            connected_ = other.connected_;
            other.socket_fd_ = -1;
            other.connected_ = false;
        }
        return *this;
    }

    bool UnixSocket::setup_address(sockaddr_un& addr) const
    {
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        
        if (socket_path_.length() >= sizeof(addr.sun_path))
        {
            return false;
        }
        
        std::strcpy(addr.sun_path, socket_path_.c_str());
        return true;
    }

    bool UnixSocket::initialize()
    {
        // Create socket
        socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socket_fd_ == -1)
        {
            return false;
        }

        sockaddr_un addr;
        if (!setup_address(addr))
        {
            close();
            return false;
        }

        if (mode_ == SERVER)
        {
            // Remove existing socket file
            unlink(socket_path_.c_str());
            
            // Bind socket
            if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
            {
                std::cerr << "Failed to bind socket " << socket_path_ << ": " << strerror(errno) << std::endl;
                close();
                return false;
            }
            
            // Listen for connections
            if (listen(socket_fd_, 5) == -1)
            {
                std::cerr << "Failed to listen on socket " << socket_path_ << ": " << strerror(errno) << std::endl;
                close();
                return false;
            }
            
            // Set socket permissions so regular users can connect
            if (chmod(socket_path_.c_str(), 0666) == -1)
            {
                std::cerr << "Failed to set permissions on socket " << socket_path_ << ": " << strerror(errno) << std::endl;
                // Continue anyway - this is not critical
            }
            
            std::cout << "Created Unix socket: " << socket_path_ << std::endl;
            connected_ = true;
        }
        else // CLIENT mode
        {
            // Connect to server
            if (connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
            {
                std::cerr << "Failed to connect to socket " << socket_path_ << ": " << strerror(errno) << std::endl;
                close();
                return false;
            }
            
            connected_ = true;
        }

        return true;
    }

    std::unique_ptr<UnixSocket> UnixSocket::accept(const int timeout_ms)
    {
        if (mode_ != SERVER || !connected_)
        {
            return nullptr;
        }

        if (timeout_ms > 0)
        {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd_, &read_fds);

            timeval timeout;
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;

            const int result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            if (result <= 0)
            {
                return nullptr; // timeout or error
            }
        }

        const int client_fd = ::accept(socket_fd_, nullptr, nullptr);
        if (client_fd == -1)
        {
            return nullptr;
        }

        return std::unique_ptr<UnixSocket>(new UnixSocket(client_fd, socket_path_));
    }

    ssize_t UnixSocket::send(const void* data, const size_t size)
    {
        if (!connected_ || socket_fd_ == -1)
        {
            return -1;
        }

        return ::send(socket_fd_, data, size, MSG_NOSIGNAL);
    }

    ssize_t UnixSocket::recv(void* buffer, const size_t size, const int timeout_ms)
    {
        if (!connected_ || socket_fd_ == -1)
        {
            return -1;
        }

        if (timeout_ms > 0)
        {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd_, &read_fds);

            timeval timeout;
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;

            const int result = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
            if (result <= 0)
            {
                return result; // 0 for timeout, -1 for error
            }
        }

        return ::recv(socket_fd_, buffer, size, 0);
    }

    std::vector<uint8_t> UnixSocket::recv_vector(const size_t max_size, const int timeout_ms)
    {
        std::vector<uint8_t> buffer(max_size);
        const ssize_t bytes_received = recv(buffer.data(), max_size, timeout_ms);
        
        if (bytes_received <= 0)
        {
            return {};
        }
        
        buffer.resize(bytes_received);
        return buffer;
    }

    void UnixSocket::close()
    {
        if (socket_fd_ != -1)
        {
            ::close(socket_fd_);
            socket_fd_ = -1;
        }
        
        if (mode_ == SERVER && !socket_path_.empty())
        {
            unlink(socket_path_.c_str());
        }
        
        connected_ = false;
    }

} // namespace HyCAN