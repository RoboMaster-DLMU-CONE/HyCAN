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

- 你可以使用`sudo`运行编译的应用程序，或者，**更推荐的**，使用PAM为你的用户添加网络接口权限。编辑
  `/etc/security/capability.conf`，添加一行：

```shell
cap_net_admin your_username
```

权限在重启后生效。

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