现代高性能 Linux C++ CAN通信协议库

## 亮点

- 基于`epoll`的高并发支持。每秒处理**100k消息**的场景下：
    - **低CPU占用**：平均 `25%` CPU占用率（基于 AMD Ryzen 7 7840HS CPU
      ），**无丢帧**
    - **低延迟**：每条消息平均延迟低至**15us**
    - [详见InterfaceStressTest](tests/InterfaceStressTest.cpp)
- 内置日志功能，调试轻松
- 用户友好的API
- 无需外置脚本，库内直接开关`Netlink`上的`CAN/VCAN`接口。

## 使用场景

- 高实时性需求
    - 控制算法依赖 CAN 总线上大量电机的实时反馈帧
- 减少部署复杂度
    - 避免每次运行程序前执行脚本手动启动 CAN/VCAN 接口

## 接入工程

### 前置依赖

- 构建工具
    - CMake ( >= 3.27 )
    - Conan ( >= 2 )
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

#### Conan安装依赖

```shell
conan install . --build=missing -s build_type=Release
```

#### CMake构建

```shell
cmake --preset conan-release
cmake --build build/Release
#可选：安装库到系统
sudo cmake --install build/Release
```

## Todo

- [ ] 更多示例和测试
- [ ] 封装常用功能的`Device`类
- [ ] 封装常用功能的`Message`类

## Credit

- [xtr](https://github.com/choll/xtr)
- [libnl](https://github.com/thom311/libnl)