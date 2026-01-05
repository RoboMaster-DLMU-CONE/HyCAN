#include <chrono>
#include <iomanip>
#include <iostream>
#include <linux/can.h>
#include <thread>

#include "HyCAN/Interface/Interface.hpp"

using HyCAN::CANInterface;
using HyCAN::VCANInterface;

int main() {
    const std::function callback = [](can_frame frame) {
        std::cout << "CAN ID: 0x" << std::hex << frame.can_id
                  << " DLC: " << std::dec << static_cast<int>(frame.len)
                  << " Data: ";
        for (int i = 0; i < frame.len; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<unsigned int>(frame.data[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    };
    constexpr can_frame test_frame = {
        .can_id = 0x100,
        .len = 8,
        .data = {0, 1, 2, 3, 4, 5, 6, 7},
    };

    std::cout << "=== HyCAN Interface Bitrate Example ===" << std::endl;

    // Example 1: VCAN interface (bitrate is ignored for virtual interfaces)
    std::cout << "\n1. Testing VCAN interface with default bitrate..."
              << std::endl;
    VCANInterface vcan_interface("vcan0");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto result =
        vcan_interface.register_callback({0x100}, callback).and_then([&] {
            std::cout
                << "Bringing up VCAN interface with default bitrate (1000000)"
                << std::endl;
            return vcan_interface
                .up(); // Uses default 1000000, but ignored for VCAN
        });
    if (!result) {
        std::cerr << "Failed to set up VCAN interface: "
                  << result.error().message << std::endl;
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Sending 3 test frames..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        (void)vcan_interface.send(test_frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    (void)vcan_interface.down();

    // Example 2: VCAN interface with custom bitrate (still ignored for VCAN)
    std::cout << "\n2. Testing VCAN interface with custom bitrate 500000..."
              << std::endl;
    VCANInterface vcan_interface2("vcan1");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    result = vcan_interface2.register_callback({0x100}, callback).and_then([&] {
        std::cout << "Bringing up VCAN interface with custom bitrate (500000)"
                  << std::endl;
        return vcan_interface2.up(
            500000); // Custom bitrate, but ignored for VCAN
    });
    if (!result) {
        std::cerr << "Failed to set up VCAN interface: "
                  << result.error().message << std::endl;
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Sending 2 test frames..." << std::endl;
    for (int i = 0; i < 2; ++i) {
        (void)vcan_interface2.send(test_frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    (void)vcan_interface2.down();

    std::cout << "\n=== Example completed ===" << std::endl;
    std::cout << "Note: For real CAN interfaces (can0, can1, etc.), "
              << std::endl;
    std::cout
        << "the bitrate parameter will be applied to configure the hardware."
        << std::endl;
    std::cout << "VCAN interfaces ignore bitrate settings as they are virtual."
              << std::endl;

    return 0;
}
