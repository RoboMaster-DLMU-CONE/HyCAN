现代高性能 Linux C++ CAN通信协议库

## 亮点

- 基于`epoll`的高并发支持。每秒100k消息的并发场景下：
    - 平均 `25%` CPU占用（ AMD Ryzen 7 7840HS
      ），无任何丢帧。
    - 每条消息平均15us延迟
    - [详见InterfaceStressTest](tests/InterfaceStressTest.cpp)
- 日志功能，随时监控CAN总线信息
- 便于使用的API接口
- 无需脚本，直接控制Netlink开关

## 接入工程

### 前置依赖

- CMake > 3.27
- Conan > 2
- GCC > 13 或 Clang > 15
- linux-modules-extra

- 使用`modprobe`激活Linux的`CAN`和`VCAN`模组：

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

- [xtr日志库](https://github.com/choll/xtr)
- [libnl](https://github.com/thom311/libnl)