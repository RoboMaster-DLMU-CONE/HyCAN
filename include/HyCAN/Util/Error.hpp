#ifndef ERROR_HPP
#define ERROR_HPP

namespace HyCAN
{
    enum class ErrorCode
    {
        // VCAN
        VCANCheckingError,

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

    struct Error
    {
        Error(const ErrorCode c, const std::string& msg): code(c), message(std::move(msg))
        {
        }

        ErrorCode code;
        std::string message;
    };
}
#endif //ERROR_HPP
