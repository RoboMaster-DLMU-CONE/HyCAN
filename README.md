现代高性能 Linux C++ CAN通信协议库

## 亮点

- 日志功能，随时监控CAN总线信息
- 便于使用的API接口
- 无需脚本，直接控制Netlink开关
- 基于epoll的高并发支持

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

### Conan安装依赖

```shell
conan install . --build=missing -s build_type=Release
```

## Todo

- [x] xtr日志库
- [x] CTest
- [ ] 线程亲和

## Credit

- [xtr日志库](https://github.com/choll/xtr)
- [libnl](https://github.com/thom311/libnl)
- [mlib](https://github.com/P-p-H-d/mlib)