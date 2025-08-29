现代高性能 Linux C++ CAN通信协议库

## 亮点

- 基于`epoll`的高并发支持。每秒处理**100k消息**的场景下：
    - **低CPU占用**：平均 `20%` CPU占用率（基于 AMD Ryzen 7 7840HS CPU
      ），**无丢帧**
    - **低延迟**：每条消息平均延迟低至**10us**
    - [详见InterfaceStressTest](tests/InterfaceStressTest.cpp)
- 用户友好的API
- **无需root权限**：通过`HyCAN Daemon`系统服务，用户程序可以普通权限运行
- **自动化接口管理**：库内通过守护进程直接开关`Netlink`上的`CAN/VCAN`接口

## 使用场景

- 高实时性需求
    - 控制算法依赖 CAN 总线上大量电机的实时反馈帧
- 减少部署复杂度和提升安全性
    - 避免每次运行程序前执行脚本手动启动 CAN/VCAN 接口
    - 消除用户应用程序对root权限的依赖
    - 通过守护进程集中管理CAN接口资源

## 接入工程

### 前置依赖

- 构建工具
    - CMake ( >= 3.20 )
- 编译器
    - GCC ( >= 13 ) 或 Clang ( >= 15 )
- 系统环境
    - Linux can 和 vcan 内核模块（通常通过 linux-modules-extra 包提供）
    - 使用`modprobe`加载内核模块：

    ```shell
    sudo modprobe vcan
    sudo modprobe can
    ```

### 从源代码构建

#### 安装依赖

```shell
# Debian系
sudo apt install pkg-config cmake libnl-3-dev libnl-nf-3-dev
```

#### CMake构建与安装

```shell
cmake -S . -B build
cmake --build build
# 安装到系统（包括HyCAN守护进程）
sudo cmake --install build
```

### 使用HyCAN

#### API使用示例

HyCAN提供了两种接口类型：

- **VCANInterface**: 用于虚拟CAN接口，自动创建和管理VCAN接口
- **CANInterface**: 用于真实CAN硬件接口，验证硬件存在

```cpp
#include <HyCAN/Interface/VCANInterface.hpp>
#include <linux/can.h>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 创建虚拟CAN接口（推荐用于测试和仿真）
    HyCAN::VCANInterface interface("vcan0");
    
    // 设置消息回调
    auto callback = [](can_frame&& frame) {
        std::cout << "收到CAN消息 ID: 0x" << std::hex << frame.can_id 
                  << " 数据长度: " << std::dec << (int)frame.len << std::endl;
    };
    
    // 注册回调并启动接口
    interface.tryRegisterCallback({0x123}, callback)
             .and_then([&] { return interface.up(); })
             .or_else([](const auto& error) {
                 std::cerr << "错误: " << error.message << std::endl;
             });
    
    // 发送CAN消息
    can_frame frame = {
        .can_id = 0x123,
        .len = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };
    
    interface.send(frame);
    
    // 等待消息处理
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 检查接口状态
    interface.is_up().and_then([](bool up) {
        std::cout << "接口状态: " << (up ? "UP" : "DOWN") << std::endl;
        return tl::expected<void, HyCAN::Error>{};
    });
    
    // 关闭接口
    interface.down();
    
    return 0;
}
```

#### 在CMake工程中使用

HyCAN支持接入CMake工程。请参考下面的`CMakeLists.txt`示例代码：

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyConsumerApp)

include(FetchContent)

# 尝试查找已安装的 HyCAN
find_package(HyCAN QUIET)

if (NOT HyCAN_FOUND)
    message(STATUS "本地未找到 HyCAN 包，尝试从 GitHub 获取...")
    FetchContent_Declare(
            HyCAN_fetched
            GIT_REPOSITORY "https://github.com/RoboMaster-DLMU-CONE/HyCAN"
            GIT_TAG "main"
    )
    FetchContent_MakeAvailable(HyCAN_fetched)
else ()
    message(STATUS "已找到 HyCAN 版本 ${HyCAN_VERSION}")
endif ()

add_executable(MyConsumerApp main.cpp)

target_link_libraries(MyConsumerApp PRIVATE HyCAN::HyCAN)
```

#### 运行你的应用

现在你的应用可以以**普通用户权限**运行：

```shell
# 无需sudo！
./MyConsumerApp
```

确保HyCAN守护进程正在运行：

```shell
# 如果守护进程未运行，启动它
sudo systemctl start hycan-daemon
```

## 架构说明

### 守护进程架构

HyCAN采用守护进程架构：

- **HyCAN Daemon**: 作为systemd服务运行，以root权限管理CAN/VCAN接口
- **用户应用**: 通过IPC与守护进程通信，无需root权限即可操作CAN接口
- **安全隔离**: 只有守护进程需要特权，用户代码运行在受限环境中

### 接口类型层次结构

HyCAN提供了清晰的接口类型层次：

- **Interface (基类)**: 提供通用的CAN通信功能
  - `up()` / `down()`: 启动/关闭接口
  - `is_up()`: 检查接口状态（新增功能）
  - `send()`: 发送CAN消息
  - `tryRegisterCallback()`: 注册消息回调
  
- **VCANInterface (派生类)**: 虚拟CAN接口
  - 自动创建和管理VCAN接口
  - 适用于测试、仿真和开发环境
  
- **CANInterface (派生类)**: 真实CAN硬件接口  
  - 验证CAN硬件存在性
  - 适用于生产环境和实际硬件

### 模块化守护进程

守护进程采用模块化设计，提高代码可维护性：

- **InterfaceManager**: 处理接口状态操作
- **CANManager**: 处理CAN特定操作（比特率设置、硬件验证）
- **RequestProcessor**: 处理不同类型的请求分发
- **Daemon**: 主协调器，处理IPC通信

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

1. HyCAN守护进程已安装并运行
2. 用户应用使用普通权限运行

## Todo

- [ ] 更多示例和测试
- [ ] 封装常用功能的`Device`类
- [ ] 封装常用功能的`Message`类

## Credit

- [libnl](https://github.com/thom311/libnl)
- [cpp-ipc](https://github.com/mutouyun/cpp-ipc)
