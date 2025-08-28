现代高性能 Linux C++ CAN通信协议库

## 亮点

- 基于`epoll`的高并发支持。每秒处理**100k消息**的场景下：
    - **低CPU占用**：平均 `20%` CPU占用率（基于 AMD Ryzen 7 7840HS CPU
      ），**无丢帧**
    - **低延迟**：每条消息平均延迟低至**10us**
    - [详见InterfaceStressTest](tests/InterfaceStressTest.cpp)
- 用户友好的API
- 无需外置脚本，库内直接开关`CAN/VCAN`接口。

## 使用场景

- 高实时性需求
    - 控制算法依赖 CAN 总线上大量电机的实时反馈帧
- 减少部署复杂度
    - 避免每次运行程序前执行脚本手动启动 CAN/VCAN 接口

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
sudo apt install cmake iproute2
```

#### CMake构建

```shell
cmake -S . -B build
cmake --build build
#可选：安装到系统（会创建sudo规则以支持无密码CAN接口管理）
sudo cmake --install build
```

#### 配置用户权限

安装后，为了让用户无需sudo密码即可管理CAN接口，请将用户添加到`dialout`组：

```shell
sudo usermod -a -G dialout $USER
# 重新登录使组权限生效
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

## Todo

- [ ] 更多示例和测试
- [ ] 封装常用功能的`Device`类
- [ ] 封装常用功能的`Message`类

## Credit

- [iproute2](https://github.com/iproute2/iproute2) - Linux 网络配置工具