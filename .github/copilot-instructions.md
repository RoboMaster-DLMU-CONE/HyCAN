# HyCAN 代码库 Copilot 编码助手指令

## 仓库概述

HyCAN 是一个现代高性能 Linux C++ CAN 通信协议库，专为高实时性需求场景设计。该库主要服务于依赖 CAN 总线上大量电机实时反馈帧的控制算法，通过减少部署复杂度来避免每次运行程序前手动启动 CAN/VCAN 接口的需求。

### 技术栈信息
- **项目类型**: C++ 静态链接库
- **语言标准**: C++20
- **构建系统**: CMake (最低版本 3.20)
- **编译器要求**: GCC >= 13 或 Clang >= 15
- **目标平台**: Linux (依赖 CAN 和 VCAN 内核模块)
- **代码规模**: 约 6,700 行代码，35 个文件 (10 个头文件，25 个源文件)

### 核心功能模块
- **Interface**: 主要 API 类，提供 CAN 通信的统一接口
- **Reaper**: 消息接收器，使用实时线程和延迟测试功能
- **Sender**: 消息发送器
- **Netlink**: CAN接口管理（使用iproute2工具）
- **Socket/VCAN**: CAN 接口管理
- **Util**: 工具类（如 SpinLock 自旋锁）

## 构建和验证说明

### 依赖安装（必须按顺序执行）

1. **系统包依赖安装**：
```bash
sudo apt update
sudo apt install -y cmake iproute2 linux-modules-extra-$(uname -r)
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

3. **运行测试**（需要 sudo 权限）：
```bash
cd build && sudo ctest --output-on-failure
```

### 构建选项说明
- `BUILD_HYCAN_TEST=ON`: 构建测试可执行文件
- `TEST_HYCAN_LATENCY=ON`: 启用延迟测试功能
- `BUILD_HYCAN_EXAMPLE=ON`: 构建示例程序（可选）

### 验证步骤
1. 所有测试应该通过（NetlinkUpDownTest, InterfaceTest, InterfaceStressTest）
2. 构建过程不应有警告或错误
3. 生成的 libHyCAN.a 静态库文件应存在于 build 目录

## 代码库布局和架构

### 主要目录结构
```
/
├── CMakeLists.txt              # 主构建配置文件
├── README.md                   # 项目文档
├── LICENSE                     # BSD 3-Clause 许可证
├── .github/workflows/test.yml  # GitHub Actions CI/CD 配置
├── cmake/                      # CMake 模块和配置
│   ├── HyCANFindTlExpected.cmake # tl::expected 依赖管理
│   ├── HyCANTests.cmake       # 测试配置
│   ├── HyCANExamples.cmake    # 示例程序配置
│   └── HyCANConfig.cmake.in   # 包配置模板
├── include/HyCAN/             # 公开头文件
│   ├── Interface/             # 接口相关头文件
│   └── Util/                  # 工具类头文件
├── src/Interface/             # 核心实现文件
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
- `include/HyCAN/Interface/Netlink.hpp`: CAN接口管理接口

### 关键设计模式
- **函数式错误处理**: 使用 `tl::expected<T, Error>` 替代异常
- **模板化接口**: `CanFrameConvertible` 概念支持多种帧类型
- **实时优化**: 内存锁定、实时调度、CPU 亲和性设置
- **线程安全**: 自定义 SpinLock 实现高性能同步

### 持续集成和验证管道
- **GitHub Actions**: 在 Ubuntu 24.04 上自动构建和测试
- **测试覆盖**: 单元测试、集成测试、压力测试
- **依赖管理**: 自动获取 tl::expected，镜像仓库故障转移

### 外部依赖说明
- **iproute2**: Linux 网络配置工具用于CAN接口管理
- **tl::expected**: 现代 C++ 错误处理库（自动从 GitHub 获取）
- **Linux CAN 内核模块**: 必须在运行时加载

### 常见问题和解决方案

1. **构建失败 - 缺少 iproute2**:
   - 确保已安装：`sudo apt install iproute2`

2. **测试失败 - CAN 模块未加载**:
   - 执行：`sudo modprobe vcan && sudo modprobe can`

3. **权限错误**:
   - 安装时创建sudo规则：`sudo cmake --install build`
   - 将用户添加到dialout组：`sudo usermod -a -G dialout $USER`

4. **网络获取失败**:
   - tl::expected 库会自动回退到 Gitee 镜像

### 开发工作流程建议

1. **代码修改前**: 始终先运行完整构建和测试验证当前状态
2. **增量开发**: 使用 `cmake --build build` 进行增量编译
3. **测试驱动**: 修改代码后立即运行相关测试
4. **实时功能**: 修改涉及 Reaper 的代码时，确保测试延迟统计功能
5. **内存安全**: 注意 CAN 帧数据的生命周期管理

### 性能敏感区域
- `src/Interface/Reaper.cpp`: 实时消息处理循环
- `include/HyCAN/Util/SpinLock.hpp`: 高频同步原语
- 延迟测试代码（`#ifdef HYCAN_LATENCY_TEST` 块）

**重要提示**: 请信任这些指令中的信息，仅在指令信息不完整或发现错误时才进行额外搜索。这些构建步骤和配置已经过验证，可以直接使用而无需重新探索。

**重要提示**: 请信任这些指令中的信息，仅在指令信息不完整或发现错误时才进行额外搜索。这些构建步骤和配置已经过验证，可以直接使用而无需重新探索。