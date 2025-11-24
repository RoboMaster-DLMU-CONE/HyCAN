# HyCAN: A Modern High-Performance Linux C++ CAN Communication Framework

[English](README.md) | [中文](README_cn.md)

A modern high-performance CAN communication protocol library for Linux, designed for scenarios with strict real-time
requirements.

## Highlights

- High concurrency based on `epoll`. In scenarios with **100k messages per second**:
    - **Low CPU usage**: average `20%` CPU usage (on AMD Ryzen 7 7840HS), **no frame loss**
    - **Low latency**: average latency as low as **10 µs** per message
    - See [InterfaceStressTest](tests/InterfaceStressTest.cpp) for details
- User-friendly API
- **No root privileges required**: with the `HyCAN Daemon` system service, user applications can run as normal users
- **Automated interface management**: the daemon manages `CAN/VCAN` interfaces on `Netlink` directly inside the library

## Use Cases

- High real-time requirements
    - Control algorithms depending on vast motor feedback frames on the CAN bus
    - Highly sensitive to message delays
- Reduced deployment complexity and improved security
    - Avoid running scripts to manually bring up CAN/VCAN interfaces before each run
    - Remove the dependency on root privileges for user applications
    - Centralized management of CAN interface resources through the daemon
    - No need to introduce other heavyweight libraries

## Integration into Your Project

### Prerequisites

- Build system
    - CMake (>= 3.20)
- Compilers
    - GCC (>= 13) or Clang (>= 15)
- System environment
    - Linux `can` and `vcan` kernel modules (usually provided by the `linux-modules-extra` package)
    - Load the kernel modules with `modprobe`:

  ```shell
  sudo modprobe vcan
  sudo modprobe can
  ```

### Install via Package Manager

Download the prebuilt packages from the [releases](https://github.com/RoboMaster-DLMU-CONE/HyCAN/releases) page.

### (Optional) Build from Source

#### Install Dependencies

```shell
# Debian-based systems
sudo apt install pkg-config cmake libnl-3-dev libnl-route-3-dev libexpected-dev
```

#### CMake Configure & Install

```shell
cmake -S . -B build
cmake --build build
# Install to the system (including the HyCAN daemon)
sudo cmake --install build
```

### Using HyCAN in CMake

HyCAN is designed to be consumed from CMake projects. A minimal `CMakeLists.txt` example:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyConsumerApp)

find_package(HyCAN REQUIRED)
add_executable(MyConsumerApp main.cpp)
target_link_libraries(MyConsumerApp PRIVATE HyCAN::HyCAN)
```

Example `main.cpp`:

```c++
#include <HyCAN/Interface/Interface.hpp>
using HyCAN::CANInterface;

int main() {
    constexpr can_frame test_frame = {
        .can_id = 0x100,
        .len = 8,
        .data = {0, 1, 2, 3, 4, 5, 6, 7},
    };

    CANInterface can_interface("can0");

    can_interface.send(test_frame);

    return 0;
}
```

#### Run Your Application

Your application can now run with **normal user privileges**:

```shell
# No sudo required!
./MyConsumerApp
```

Make sure the HyCAN daemon is running:

```shell
# Start the daemon if it is not running
sudo systemctl start hycan-daemon
```

#### More Examples

See the programs under the [example](example) directory.

## Architecture Overview

HyCAN uses a daemon-based architecture:

- **HyCAN Daemon**: runs as a `systemd` service with root privileges to manage CAN/VCAN interfaces
- **User applications**: communicate with the daemon via IPC and can operate CAN interfaces without root privileges
- **Security isolation**: only the daemon needs elevated privileges; user code runs in a restricted environment

## Troubleshooting

### Daemon-related Issues

```shell
# Check daemon status
sudo systemctl status hycan-daemon

# View daemon logs
sudo journalctl -u hycan-daemon -f

# Restart the daemon
sudo systemctl restart hycan-daemon
```

### Permission Issues

If you encounter permission-related errors, make sure:

1. The HyCAN daemon is installed and running
2. Your application is running as a normal user (no sudo required)

## Todo

- [ ] More examples and tests
- [ ] Native Linux CAN filter support
- [ ] `Device` class wrapping common functionality
- [ ] `Message` class wrapping common functionality

## Credit

- [libnl](https://github.com/thom311/libnl)
