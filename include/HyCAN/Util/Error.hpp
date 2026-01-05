#ifndef HYCAN_ERROR_HPP
#define HYCAN_ERROR_HPP

#include <string>

namespace HyCAN {
enum class ErrorCode {
    // VCAN
    VCANCheckingError,
    VCANCreationError,

    // Netlink
    NlSocketAllocError,
    NlConnectError,
    RtnlLinkAllocError,
    RtnlLinkAddError,
    NetlinkSocketNotInitialized,
    NetlinkConnectError,
    NetlinkInterfaceNotFound,
    NetlinkBringDownError,
    NetlinkBringUpError,

    // Socket
    CANSocketCreateError,
    CANInterfaceIndexError,
    CANSocketBindError,
    CANSocketWriteError,
    CANSocketBufferFull,
    CANInvalidSocketError,
    CANFlushError,

    // Reaper
    EpollError,
    ReaperStopError,
    MemoryLockError,
    ThreadRealTimeError,
    CPUAffinityError,
    EmptyFuncError,
    FuncCANIdSetError,
};

struct Error {
    Error(const ErrorCode c, const std::string &msg)
        : code(c), message(std::move(msg)) {}

    ErrorCode code;
    std::string message;
};
} // namespace HyCAN
#endif // HYCAN_ERROR_HPP
