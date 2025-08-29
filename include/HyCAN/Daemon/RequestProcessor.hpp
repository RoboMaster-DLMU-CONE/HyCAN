#ifndef HYCAN_REQUEST_PROCESSOR_HPP
#define HYCAN_REQUEST_PROCESSOR_HPP

namespace HyCAN
{
    struct NetlinkRequest;
    struct NetlinkResponse;
    class InterfaceManager;
    class CANManager;

    /**
     * @brief Processes different types of daemon requests
     */
    class RequestProcessor
    {
        const InterfaceManager& interface_manager_;
        const CANManager& can_manager_;

    public:
        RequestProcessor(const InterfaceManager& interface_mgr, const CANManager& can_mgr);

        NetlinkResponse process_request(const NetlinkRequest& request) const;
    };
}

#endif