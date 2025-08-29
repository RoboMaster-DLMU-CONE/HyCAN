#ifndef VCAN_INTERFACE_HPP
#define VCAN_INTERFACE_HPP

#include "Interface.hpp"

namespace HyCAN
{
    /**
     * @brief VCAN interface class for managing virtual CAN interfaces
     */
    class VCANInterface : public Interface
    {
    public:
        explicit VCANInterface(const std::string& interface_name);
        VCANInterface() = delete;

        // Override virtual methods from base class
        tl::expected<void, Error> up() override;
    };
}

#endif //VCAN_INTERFACE_HPP