现代高性能 Linux C++ CAN通信协议库

## 亮点

- 日志功能，随时监控CAN总线信息
- 支持liburing，适用于高频CAN总线
- 简洁的纯模块设计，加快编译速度

## 接入工程

### 前置依赖

- CMake > 3.31
- Clang = 19
- Conan > 2
- linux-modules-extra

使用`modprobe`激活Linux的`CAN`和`VCAN`模组：

```shell
sudo modprobe vcan
sudo modprobe can
```

### Conan安装依赖

```shell
conan install . --build=missing
```
你可能需要调整使用的编译器。用`export CC=clang-19`来导出环境变量；

## Todo

- [x] xtr日志库
- [x] CTest

## Credit

- [xtr日志库](https://github.com/choll/xtr)