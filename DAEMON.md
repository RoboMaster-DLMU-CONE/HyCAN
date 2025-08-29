# HyCAN Daemon Architecture

## Overview

HyCAN now uses a daemon architecture where all privileged network operations (requiring CAP_NET_ADMIN) are performed by a separate `hycan_daemon` process. The HyCAN library communicates with this daemon via Unix domain sockets.

## Architecture Benefits

- **Security**: Only the daemon requires elevated privileges
- **Isolation**: Application code runs with minimal privileges
- **Service Integration**: Daemon can be managed as a systemd service
- **Reliability**: Daemon handles multiple client connections

## Components

### hycan_daemon
The daemon executable that handles:
- VCAN interface creation
- Network interface up/down operations
- Unix socket server for IPC

### DaemonClient
Client library component that:
- Connects to daemon via Unix socket
- Sends interface operation requests
- Handles error responses

### Netlink (Modified)
The existing Netlink class now:
- Uses DaemonClient instead of direct libnl3 calls
- Maintains the same public API
- Provides backward compatibility

## Installation

### Building with Daemon Support

```bash
cmake -S . -B build -DBUILD_HYCAN_DAEMON=ON -DBUILD_HYCAN_TEST=ON
cmake --build build
```

### Installing as System Service

```bash
sudo cmake --install build
# This will:
# - Install hycan_daemon to /usr/local/bin/
# - Install systemd service file
# - Enable and start the service
```

### Manual Daemon Management

```bash
# Start daemon manually
sudo hycan_daemon [socket_path]

# Default socket path: /tmp/hycan_daemon.sock
# System service socket: /var/run/hycan_daemon.sock
```

## Socket Path Configuration

The library automatically selects the appropriate socket path:

1. `HYCAN_DAEMON_SOCKET` environment variable (if set)
2. `/var/run/hycan_daemon.sock` (if system service is running)
3. `/tmp/hycan_daemon.sock` (default fallback)

## Testing

### With Daemon Management

```bash
cd build
sudo ../scripts/test_with_daemon.sh
```

This script:
- Starts the daemon
- Runs all tests
- Cleans up automatically

### Manual Testing

```bash
# Terminal 1: Start daemon
sudo ./hycan_daemon

# Terminal 2: Run tests
sudo ctest --output-on-failure
```

## Systemd Service

The daemon can be managed as a systemd service:

```bash
# Check status
systemctl status hycan-daemon

# Start/stop/restart
sudo systemctl start hycan-daemon
sudo systemctl stop hycan-daemon
sudo systemctl restart hycan-daemon

# Enable/disable autostart
sudo systemctl enable hycan-daemon
sudo systemctl disable hycan-daemon
```

## IPC Protocol

The daemon uses a simple binary protocol over Unix domain sockets:

### Request Format
```
[operation_type:1][interface_name_length:1][interface_name:variable]
```

### Response Format
```
[success:1][error_code:1][message_length:2][message:variable]
```

### Operations
- `CREATE_VCAN = 1`: Create VCAN interface
- `INTERFACE_UP = 2`: Bring interface up
- `INTERFACE_DOWN = 3`: Bring interface down

## Error Handling

- Library gracefully handles daemon unavailability
- Clear error messages for connection failures
- Automatic fallback behavior
- Proper resource cleanup

## Performance

The daemon architecture maintains excellent performance:
- Average latency: ~20μs (stress test results)
- Supports high-frequency operations
- Minimal IPC overhead
- Concurrent client support