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

HyCAN采用守护进程架构：

- **HyCAN Daemon**: 作为systemd服务运行，以root权限管理CAN/VCAN接口
- **用户应用**: 通过IPC与守护进程通信，无需root权限即可操作CAN接口
- **安全隔离**: 只有守护进程需要特权，用户代码运行在受限环境中

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
