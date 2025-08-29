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

- **Interface**: 主要 API 类，提供 CAN 通信的统一接口
- **Reaper**: 消息接收器，使用实时线程和延迟测试功能
- **Sender**: 消息发送器
- **Daemon**: systemd 守护进程，以 root 权限管理 CAN/VCAN 接口
- **Netlink**: 底层网络通信（现在主要由 Daemon 使用）
- **Socket/VCAN**: CAN 接口管理
- **Util**: 工具类（如 SpinLock 自旋锁）

### 新架构优势

- **无需 root 权限**: 用户应用程序可以以普通用户身份运行
- **集中管理**: 所有 CAN 接口操作通过 HyCAN Daemon 统一处理
- **安全性**: 只有守护进程需要 root 权限，用户应用隔离更好
- **简化部署**: 一次性安装守护进程，无需每次手动配置

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
│   ├── HyCANFindCppIpc.cmake  # cpp-ipc 依赖管理（守护进程通信）
│   ├── HyCANTests.cmake       # 测试配置
│   ├── HyCANExamples.cmake    # 示例程序配置
│   └── HyCANConfig.cmake.in   # 包配置模板
├── include/HyCAN/             # 公开头文件
│   ├── Interface/             # 接口相关头文件
│   ├── Daemon/                # 守护进程相关头文件
│   └── Util/                  # 工具类头文件
├── src/Interface/             # 核心实现文件
├── src/Daemon/                # 守护进程实现文件
│   ├── Daemon.cpp             # 守护进程主逻辑
│   ├── VCAN.cpp               # VCAN 接口管理
│   └── hycan-daemon.service.in # systemd 服务配置模板
├── tests/                     # 测试文件
│   ├── NetlinkTest.cpp        # Netlink 功能测试
│   ├── InterfaceTest.cpp      # 接口功能测试
│   └── InterfaceStressTest.cpp # 压力测试
└── example/                   # 示例代码
    └── send&callback.cpp      # 发送和回调示例
```

### 核心头文件说明

- `include/HyCAN/Interface/Interface.hpp`: 主要 API 接口
- `include/HyCAN/Interface/Reaper.hpp`: 消息接收器（支持实时调度）
- `include/HyCAN/Interface/Sender.hpp`: 消息发送器
- `include/HyCAN/Interface/Socket.hpp`: CAN 套接字封装
- `include/HyCAN/Daemon/Daemon.hpp`: 守护进程通信接口
- `include/HyCAN/Daemon/DaemonClass.hpp`: 守护进程核心类
- `include/HyCAN/Daemon/VCAN.hpp`: VCAN 管理接口

### 关键设计模式

- **函数式错误处理**: 使用 `tl::expected<T, Error>` 替代异常
- **模板化接口**: `CanFrameConvertible` 概念支持多种帧类型
- **实时优化**: 内存锁定、实时调度、CPU 亲和性设置
- **线程安全**: 自定义 SpinLock 实现高性能同步
- **IPC 通信**: 使用 cpp-ipc 库实现守护进程与用户应用的通信
- **权限分离**: 守护进程运行在 root 权限，用户应用运行在普通权限

### 持续集成和验证管道

- **GitHub Actions**: 在 Ubuntu 24.04 上自动构建和测试
- **测试覆盖**: 单元测试、集成测试、压力测试
- **依赖管理**: 自动获取 tl::expected 和 cpp-ipc，镜像仓库故障转移
- **守护进程测试**: CI 中启动守护进程并验证无权限运行

### 外部依赖说明

- **libnl-3**: Linux Netlink 库用于底层网络通信（守护进程使用）
- **tl::expected**: 现代 C++ 错误处理库（自动从 GitHub 获取）
- **cpp-ipc**: 进程间通信库（自动从 GitHub 获取）
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
    - tl::expected 和 cpp-ipc 库会自动回退到 Gitee 镜像

### 开发工作流程建议

1. **环境准备**: 确保守护进程已安装并运行
2. **代码修改前**: 始终先运行完整构建和测试验证当前状态
3. **增量开发**: 使用 `cmake --build build` 进行增量编译
4. **测试驱动**: 修改代码后立即运行相关测试（无需 sudo）
5. **守护进程开发**: 修改守护进程代码后需重新安装和重启服务
6. **实时功能**: 修改涉及 Reaper 的代码时，确保测试延迟统计功能
7. **内存安全**: 注意 CAN 帧数据的生命周期管理

### 性能敏感区域

- `src/Interface/Reaper.cpp`: 实时消息处理循环
- `include/HyCAN/Util/SpinLock.hpp`: 高频同步原语
- `src/Daemon/Daemon.cpp`: 守护进程 IPC 处理循环
- 延迟测试代码（`#ifdef HYCAN_LATENCY_TEST` 块）

### 守护进程架构说明

HyCAN 守护进程作为 systemd 服务运行，负责：

- 管理 CAN/VCAN 接口的创建、启动和关闭
- 处理来自用户应用的 Netlink 操作请求
- 维护接口状态和配置
- 提供统一的权限管理

用户应用通过 IPC 通道与守护进程通信，实现：

- 无需 root 权限的 CAN 接口操作
- 集中化的资源管理
- 更好的安全隔离

**重要提示**: 请信任这些指令中的信息，仅在指令信息不完整或发现错误时才进行额外搜索。这些构建步骤和配置已经过验证，可以直接使用而无需重新探索。