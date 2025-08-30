#ifndef HYCAN_UTIL_UNIX_SOCKET_HPP
#define HYCAN_UTIL_UNIX_SOCKET_HPP

#include <string>
#include <vector>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>

namespace HyCAN
{
    /**
     * @brief Unix domain socket wrapper for IPC communication
     */
    class UnixSocket
    {
    public:
        enum Mode
        {
            SERVER = 1,
            CLIENT = 2
        };

    private:
        int socket_fd_{-1};
        std::string socket_path_;
        Mode mode_;
        bool connected_{false};

    public:
        /**
         * @brief Constructor for Unix domain socket
         * @param path Socket path (will be prefixed with /run/hycan_)
         * @param mode SERVER or CLIENT mode
         */
        UnixSocket(const std::string& path, Mode mode);
        
        /**
         * @brief Destructor - automatically closes socket
         */
        ~UnixSocket();

        /**
         * @brief Initialize the socket (bind for server, connect for client)
         * @return true on success, false on failure
         */
        bool initialize();

        /**
         * @brief Accept incoming connection (server mode only)
         * @param timeout_ms Timeout in milliseconds (0 = no timeout)
         * @return New UnixSocket for the accepted connection, or nullptr on failure/timeout
         */
        std::unique_ptr<UnixSocket> accept(int timeout_ms = 0);

        /**
         * @brief Send data through the socket
         * @param data Data to send
         * @param size Size of data
         * @return Number of bytes sent, or -1 on error
         */
        ssize_t send(const void* data, size_t size);

        /**
         * @brief Receive data from the socket
         * @param buffer Buffer to receive data into
         * @param size Maximum size to receive
         * @param timeout_ms Timeout in milliseconds (0 = no timeout)
         * @return Number of bytes received, 0 on timeout, -1 on error
         */
        ssize_t recv(void* buffer, size_t size, int timeout_ms = 0);

        /**
         * @brief Receive data as vector
         * @param max_size Maximum size to receive
         * @param timeout_ms Timeout in milliseconds
         * @return Vector with received data (empty on timeout/error)
         */
        std::vector<uint8_t> recv_vector(size_t max_size, int timeout_ms = 0);

        /**
         * @brief Close the socket
         */
        void close();

        /**
         * @brief Check if socket is connected
         */
        bool is_connected() const { return connected_; }

        /**
         * @brief Get socket file descriptor
         */
        int get_fd() const { return socket_fd_; }

        // Non-copyable
        UnixSocket(const UnixSocket&) = delete;
        UnixSocket& operator=(const UnixSocket&) = delete;

        // Movable
        UnixSocket(UnixSocket&& other) noexcept;
        UnixSocket& operator=(UnixSocket&& other) noexcept;

    private:
        /**
         * @brief Constructor for accepted connections
         */
        UnixSocket(int fd, const std::string& path);

        /**
         * @brief Setup socket address structure
         */
        bool setup_address(sockaddr_un& addr) const;
    };

} // namespace HyCAN

#endif