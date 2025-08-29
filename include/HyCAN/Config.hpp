#ifndef HYCAN_CONFIG_HPP
#define HYCAN_CONFIG_HPP

#include <string>
#include <cstdlib>
#include <unistd.h>

namespace HyCAN
{
    namespace Config
    {
        // Default socket path for daemon communication
        constexpr const char* DEFAULT_DAEMON_SOCKET_PATH = "/tmp/hycan_daemon.sock";
        
        // Production socket path (when installed as system service)
        constexpr const char* SYSTEM_DAEMON_SOCKET_PATH = "/var/run/hycan_daemon.sock";
        
        // Get the appropriate socket path based on environment
        inline std::string get_daemon_socket_path()
        {
            // Check if we're running as system service
            if (const char* env_path = getenv("HYCAN_DAEMON_SOCKET"))
            {
                return std::string(env_path);
            }
            
            // Check if system socket exists
            if (access(SYSTEM_DAEMON_SOCKET_PATH, F_OK) == 0)
            {
                return std::string(SYSTEM_DAEMON_SOCKET_PATH);
            }
            
            // Fall back to default
            return std::string(DEFAULT_DAEMON_SOCKET_PATH);
        }
    }
}

#endif //HYCAN_CONFIG_HPP