# HyCAN 代码库 Copilot 编码助手指令

## 仓库概述

HyCAN 是一个现代高性能 Linux C++ CAN 通信协议库，专为高实时性需求场景设计。该库主要服务于依赖 CAN 总线上大量电机实时反馈帧的控制算法，通过使用
systemd 守护进程架构来避免每次运行程序前手动启动 CAN/VCAN 接口的需求，并消除了用户应用程序对 root 权限的依赖。

### 技术栈信息

- **项目类型**: C++ 静态链接库 + systemd 守护进程
- **语言标准**: C++20
- **构建系统**: CMake (最低版本 3.20)
- **编译器要求**: GCC >= 13 或 Clang >= 15
- **目标平台**: Linux (依赖 CAN 和 VCAN 内核模块)
- **代码规模**: 约 8,000 行代码，40+ 个文件 (15+ 个头文件，25+ 个源文件)

### 核心功能模块

- **Interface**: 主要 API 类，提供模板化的 CAN 通信统一接口，支持 CAN/VCAN 类型
- **Reaper**: 消息接收器，使用实时线程和延迟测试功能
- **Sender**: 消息发送器
- **Daemon**: systemd 守护进程，以 root 权限管理 CAN/VCAN 接口
- **Netlink**: 底层网络通信单例类（客户端侧），与守护进程通信
- **NetlinkClient**: 专用客户端通信类，支持比特率配置
- **NetlinkManager**: 守护进程侧的网络接口管理
- **UnixSocket**: 自定义 Unix 域套接字实现，替代 cpp-ipc
- **Socket/VCAN**: CAN 接口管理
- **Util**: 工具类（如 SpinLock 自旋锁）

### 新架构优势

- **无需 root 权限**: 用户应用程序可以以普通用户身份运行
- **集中管理**: 所有 CAN 接口操作通过 HyCAN Daemon 统一处理
- **安全性**: 只有守护进程需要 root 权限，用户应用隔离更好
- **简化部署**: 一次性安装守护进程，无需每次手动配置
- **稳定通信**: 使用自定义 Unix 域套接字，消除 cpp-ipc 分段错误
- **类型安全**: 模板化接口提供编译时类型检查
- **灵活配置**: 支持可配置的 CAN 比特率设置

## 构建和验证说明

### 依赖安装（必须按顺序执行）

1. **系统包依赖安装**：

```bash
sudo apt update
sudo apt install -y pkg-config libnl-3-dev libnl-nf-3-dev linux-modules-extra-$(uname -r)
```

2. **加载内核模块**（构建前始终必须执行）：

```bash
sudo modprobe vcan
sudo modprobe can
```

### 构建流程（已验证有效）

1. **配置构建**：

```bash
cmake -S . -B build -DBUILD_HYCAN_TEST=ON -DTEST_HYCAN_LATENCY=ON
```

2. **编译项目**：

```bash
cmake --build build
```

3. **安装 HyCAN（包括守护进程）**：

```bash
sudo cmake --install build
```

4. **启动并启用 HyCAN 守护进程**：

```bash
sudo systemctl enable --now hycan-daemon
```

5. **运行测试**（现在无需 sudo 权限）：

```bash
cd build && ctest --output-on-failure
```

### 构建选项说明

- `BUILD_HYCAN_TEST=ON`: 构建测试可执行文件
- `TEST_HYCAN_LATENCY=ON`: 启用延迟测试功能
- `BUILD_HYCAN_EXAMPLE=ON`: 构建示例程序（可选）

### 验证步骤

1. 守护进程应该正在运行：`sudo systemctl status hycan-daemon`
2. 所有测试应该通过（NetlinkUpDownTest, InterfaceTest, InterfaceStressTest）
3. 构建过程不应有警告或错误
4. 生成的 libHyCAN.a 静态库文件应存在于 build 目录
5. 用户应用程序可以以普通用户权限运行

## 代码库布局和架构

### 主要目录结构

```
/
├── CMakeLists.txt              # 主构建配置文件
├── README.md                   # 项目文档
├── LICENSE                     # BSD 3-Clause 许可证
├── .github/workflows/test.yml  # GitHub Actions CI/CD 配置
├── cmake/                      # CMake 模块和配置
│   ├── HyCANFindLibnl3.cmake  # libnl3 依赖查找
│   ├── HyCANFindTlExpected.cmake # tl::expected 依赖管理
│   ├── HyCANInstall.cmake     # 安装配置（包含 UnixSocket）
│   ├── HyCANTests.cmake       # 测试配置
│   ├── HyCANExamples.cmake    # 示例程序配置
│   └── HyCANConfig.cmake.in   # 包配置模板
├── include/HyCAN/             # 公开头文件
│   ├── Interface/             # 接口相关头文件
│   │   ├── Interface.hpp      # 模板化主接口类
│   │   ├── Netlink.hpp        # 客户端 Netlink 单例
│   │   ├── NetlinkClient.hpp  # 专用客户端通信
│   │   ├── Reaper.hpp         # 消息接收器
│   │   ├── Sender.hpp         # 消息发送器
│   │   ├── Socket.hpp         # CAN 套接字封装
│   │   └── CanFrameConvertible.hpp # 帧类型概念
│   ├── Daemon/                # 守护进程相关头文件
│   │   ├── Daemon.hpp         # 守护进程通信接口
│   │   ├── NetlinkManager.hpp # 守护进程 Netlink 管理
│   │   ├── UnixSocket/        # Unix 域套接字实现
│   │   │   └── UnixSocket.hpp # 套接字头文件
│   │   ├── Message.hpp        # IPC 消息定义
│   │   └── VCAN.hpp           # VCAN 管理接口
│   └── Util/                  # 工具类头文件
│       ├── Error.hpp          # 错误处理
│       └── SpinLock.hpp       # 自旋锁
├── src/Interface/             # 核心实现文件
│   ├── Interface.cpp          # 模板化接口实现
│   ├── Netlink.cpp            # 客户端 Netlink 实现
│   ├── NetlinkClient.cpp      # 客户端通信实现
│   ├── Reaper.cpp             # 消息接收器实现
│   ├── Sender.cpp             # 消息发送器实现
│   └── Socket.cpp             # 套接字实现
├── src/Daemon/                # 守护进程实现文件
│   ├── Daemon.cpp             # 守护进程主逻辑
│   ├── NetlinkManager.cpp     # 守护进程 Netlink 管理
│   ├── UnixSocket/            # Unix 域套接字实现
│   │   └── UnixSocket.cpp     # 套接字实现
│   ├── VCAN.cpp               # VCAN 接口管理
│   ├── main.cpp               # 守护进程入口
│   └── systemed/              # systemd 配置
│       └── hycan-daemon.service.in # 服务配置模板
├── tests/                     # 测试文件
│   ├── NetlinkTest.cpp        # Netlink 功能测试
│   ├── InterfaceTest.cpp      # 接口功能测试
│   └── InterfaceStressTest.cpp # 压力测试
└── example/                   # 示例代码
    └── send&callback.cpp      # 发送和回调示例
```

### 核心头文件说明

- `include/HyCAN/Interface/Interface.hpp`: 模板化主 API 接口，支持 CAN/VCAN 类型
- `include/HyCAN/Interface/Netlink.hpp`: 客户端 Netlink 单例类
- `include/HyCAN/Interface/NetlinkClient.hpp`: 专用客户端通信类，支持比特率配置
- `include/HyCAN/Interface/Reaper.hpp`: 消息接收器（支持实时调度）
- `include/HyCAN/Interface/Sender.hpp`: 消息发送器
- `include/HyCAN/Interface/Socket.hpp`: CAN 套接字封装
- `include/HyCAN/Daemon/Daemon.hpp`: 守护进程通信接口
- `include/HyCAN/Daemon/NetlinkManager.hpp`: 守护进程网络接口管理
- `include/HyCAN/Daemon/UnixSocket/UnixSocket.hpp`: 自定义 Unix 域套接字
- `include/HyCAN/Daemon/VCAN.hpp`: VCAN 管理接口

### 关键设计模式

- **函数式错误处理**: 使用 `tl::expected<T, Error>` 替代异常
- **模板化接口**: `CanFrameConvertible` 概念支持多种帧类型，Interface 类型安全模板
- **实时优化**: 内存锁定、实时调度、CPU 亲和性设置
- **线程安全**: 自定义 SpinLock 实现高性能同步
- **Unix 域套接字通信**: 自定义 UnixSocket 实现替代 cpp-ipc，消除分段错误
- **权限分离**: 守护进程运行在 root 权限，用户应用运行在普通权限
- **模块化架构**: NetlinkClient 分离，Netlink 单例管理，清晰的职责分工

### 持续集成和验证管道

- **GitHub Actions**: 在 Ubuntu 24.04 上自动构建和测试
- **测试覆盖**: 单元测试、集成测试、压力测试
- **依赖管理**: 自动获取 tl::expected，不再依赖 cpp-ipc
- **守护进程测试**: CI 中启动守护进程并验证无权限运行

### 外部依赖说明

- **libnl-3**: Linux Netlink 库用于底层网络通信（守护进程使用）
- **tl::expected**: 现代 C++ 错误处理库（自动从 GitHub 获取）
- **Linux CAN 内核模块**: 必须在运行时加载

### 常见问题和解决方案

1. **构建失败 - 缺少 libnl3**:
    - 确保已安装：`sudo apt install libnl-3-dev libnl-nf-3-dev`

2. **测试失败 - CAN 模块未加载**:
    - 执行：`sudo modprobe vcan && sudo modprobe can`

3. **守护进程未运行**:
    - 检查状态：`sudo systemctl status hycan-daemon`
    - 启动服务：`sudo systemctl start hycan-daemon`
    - 查看日志：`sudo journalctl -u hycan-daemon -f`

4. **权限错误（旧版本兼容）**:
    - 新架构下用户应用无需 sudo 权限
    - 确保守护进程正在运行：`sudo systemctl is-active hycan-daemon`

5. **网络获取失败**:
    - tl::expected 库会自动回退到 Gitee 镜像

6. **接口类型错误**:
    - 使用 `VCANInterface` 和 `CANInterface` 类型别名
    - VCAN 接口自动创建，CAN 接口必须预先存在

### 开发工作流程建议

1. **环境准备**: 确保守护进程已安装并运行
2. **代码修改前**: 始终先运行完整构建和测试验证当前状态
3. **增量开发**: 使用 `cmake --build build` 进行增量编译
4. **测试驱动**: 修改代码后立即运行相关测试（无需 sudo）
5. **守护进程开发**: 修改守护进程代码后需重新安装和重启服务
6. **实时功能**: 修改涉及 Reaper 的代码时，确保测试延迟统计功能
7. **内存安全**: 注意 CAN 帧数据的生命周期管理
8. **接口模板**: 使用 `VCANInterface` 和 `CANInterface` 类型别名
9. **比特率配置**: 新的 `up(bitrate)` 方法支持自定义比特率
10. **通信稳定性**: UnixSocket 实现消除了 cpp-ipc 分段错误

### 性能敏感区域

- `src/Interface/Reaper.cpp`: 实时消息处理循环
- `include/HyCAN/Util/SpinLock.hpp`: 高频同步原语
- `src/Daemon/Daemon.cpp`: 守护进程 Unix 套接字处理循环
- `src/Daemon/UnixSocket/UnixSocket.cpp`: 高性能套接字通信
- `src/Interface/NetlinkClient.cpp`: 客户端通信逻辑
- 延迟测试代码（`#ifdef HYCAN_LATENCY_TEST` 块）

### 守护进程架构说明

HyCAN 守护进程作为 systemd 服务运行，负责：

- 管理 CAN/VCAN 接口的创建、启动和关闭
- 处理来自用户应用的 Netlink 操作请求
- 维护接口状态和配置
- 提供统一的权限管理
- 支持可配置的 CAN 比特率设置

用户应用通过 Unix 域套接字与守护进程通信，实现：

- 无需 root 权限的 CAN 接口操作
- 集中化的资源管理
- 更好的安全隔离
- 稳定的通信（消除 cpp-ipc 分段错误）

### 新的通信架构

1. **主套接字**: `/run/hycan_daemon` 用于客户端注册
2. **客户端套接字**: `/run/hycan_HyCAN_Client_{PID}` 用于专用通信
3. **权限设置**: 套接字创建时设置 0666 权限供用户访问
4. **进程隔离**: 每个客户端进程有独立的通信通道

### Interface 模板化 API

```cpp
// 使用类型别名创建接口
VCANInterface vcan("vcan0");  // 虚拟 CAN 接口
CANInterface can("can0");     // 物理 CAN 接口

// 带比特率的接口启动
vcan.up();                    // 默认 1000000，VCAN 忽略比特率
can.up(500000);              // 500kbps CAN 比特率
can.up();                     // 默认 1000000 比特率

// 模板化发送和回调
vcan.send(frame);
vcan.tryRegisterCallback({0x100}, callback);
```

**重要提示**: 请信任这些指令中的信息，仅在指令信息不完整或发现错误时才进行额外搜索。这些构建步骤和配置已经过验证，可以直接使用而无需重新探索。