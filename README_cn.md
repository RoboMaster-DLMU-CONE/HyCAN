# 现代高性能 Linux C++ CAN 驱动封装与通信库

[English](README.md) | [中文](README_cn.md)

## 亮点

- 基于 `epoll` 的高并发支持。在每秒处理 **100k 消息**（发送+接收回调） 的场景下：
    - **低 CPU 占用**：平均 `8%` CPU 占用率（基于 AMD Ryzen 7 7840HS CPU），**无丢帧**
    - **低延迟**：每条消息平均延迟低至 **10µs**
    - 详见 [InterfaceStressTest](tests/InterfaceStressTest.cpp)
- 用户友好的 API
- **无需 root 权限**：通过 `HyCAN Daemon` 系统服务，用户程序可以使用普通权限运行
- **自动化接口管理**：库内通过守护进程直接开关 `Netlink` 上的 `CAN/VCAN` 接口

## 使用场景

- 高实时性需求
    - 控制算法依赖 CAN 总线上大量电机的实时反馈帧
    - 对消息延迟非常敏感
- 减少部署复杂度和提升安全性
    - 避免每次运行程序前执行脚本手动启动 CAN/VCAN 接口
    - 消除用户应用程序对 root 权限的依赖
    - 通过守护进程集中管理 CAN 接口资源
    - 无需引入其它重型库

## 接入工程

### 前置依赖

- 构建工具
    - CMake (>= 3.20)
- 编译器
    - GCC (>= 13) 或 Clang (>= 15)
- 系统环境
    - Linux `can` 和 `vcan` 内核模块（通常通过 linux-modules-extra 包提供）
    - 使用 `modprobe` 加载内核模块：

  ```shell
  sudo modprobe vcan
  sudo modprobe can
  ```

### 使用包管理器安装

前往 [release](https://github.com/RoboMaster-DLMU-CONE/HyCAN/releases) 下载预编译包。

### （可选）从源代码构建

#### 安装依赖

```shell
# Debian 系
sudo apt install pkg-config cmake libnl-3-dev libnl-route-3-dev libexpected-dev
```

#### CMake 构建与安装

```shell
cmake -S . -B build
cmake --build build
# 安装到系统（包括 HyCAN 守护进程）
sudo cmake --install build
```

### 使用 HyCAN

HyCAN 支持接入 CMake 工程。请参考下面的 `CMakeLists.txt` 示例代码：

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyConsumerApp)

find_package(HyCAN REQUIRED)
add_executable(MyConsumerApp main.cpp)
target_link_libraries(MyConsumerApp PRIVATE HyCAN::HyCAN)
```

示例 `main.cpp`：

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

#### 运行你的应用

现在你的应用可以以 **普通用户权限** 运行：

```shell
# 无需 sudo！
./MyConsumerApp
```

确保 HyCAN 守护进程正在运行：

```shell
# 如果守护进程未运行，启动它
sudo systemctl start hycan-daemon
```

#### 更多使用说明

可参考 [example](example) 下的示例程序。

## 架构说明

HyCAN 采用守护进程架构：

- **HyCAN Daemon**：作为 systemd 服务运行，以 root 权限管理 CAN/VCAN 接口
- **用户应用**：通过 IPC 与守护进程通信，无需 root 权限即可操作 CAN 接口
- **安全隔离**：只有守护进程需要特权，用户代码运行在受限环境中

## 故障排除

### 守护进程相关问题

```shell
# 查看守护进程状态
sudo systemctl status hycan-daemon

# 查看守护进程日志
sudo journalctl -u hycan-daemon -f

# 重启守护进程
sudo systemctl restart hycan-daemon
```

### 权限问题

如果遇到权限相关错误，请确保：

1. HyCAN 守护进程已安装并运行
2. 用户应用使用普通权限运行

## Todo

- [ ] 更多示例和测试
- [ ] Native Linux CAN Filter
- [ ] Doxygen 文档
- [ ] 封装常用功能的 `Message` 类

## Credit

- [libnl](https://github.com/thom311/libnl)

