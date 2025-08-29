#ifndef CAN_INTERFACE_HPP
#define CAN_INTERFACE_HPP

#include "Interface.hpp"

namespace HyCAN
{
    /**
     * @brief CAN interface class for managing real CAN hardware interfaces
     */
    class CANInterface : public Interface
    {
    public:
        explicit CANInterface(const std::string& interface_name);
        CANInterface() = delete;

        // Override virtual methods from base class
        tl::expected<void, Error> up() override;
        
    private:
        // Validate that the CAN hardware interface exists
        tl::expected<void, Error> validate_can_hardware();
    };
}

#endif //CAN_INTERFACE_HPP