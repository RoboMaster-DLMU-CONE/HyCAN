#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <linux/can.h>
#include <thread>

#include "HyCAN/Interface/Interface.hpp"

using HyCAN::CANInterface;

// Atomic flag to handle graceful shutdown on Ctrl+C
std::atomic<bool> keep_running(true);

void signal_handler(int signum) {
    if (signum == SIGINT) {
        std::cout << "\nCaught signal, shutting down..." << std::endl;
        keep_running = false;
    }
}

int main() {
    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "=== HyCAN Real CAN Interface Example ===" << std::endl;
    std::cout << "This example uses the 'can0' interface." << std::endl;
    std::cout << "Make sure 'can0' exists and is properly configured, or that "
                 "you have a CAN device connected."
              << std::endl;
    std::cout << "The program will attempt to bring up 'can0' with a bitrate "
                 "of 1000000."
              << std::endl;
    std::cout << "Press Ctrl+C to exit." << std::endl << std::endl;

    // 1. Define a callback function to print received frames
    const std::function callback = [](can_frame frame) {
        std::cout << "Received CAN Frame -> ID: 0x" << std::hex << frame.can_id
                  << ", DLC: " << std::dec << static_cast<int>(frame.len)
                  << ", Data: ";
        for (int i = 0; i < frame.len; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<unsigned int>(frame.data[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    };

    // 2. Create a CANInterface instance for "can0"
    CANInterface can_interface("can0");

    // 3. Register the callback for all CAN IDs and bring up the interface
    // The callback will receive all frames since we provide an empty set of
    // IDs.
    auto result =
        can_interface.register_callback({0x123}, callback).and_then([&] {
            std::cout << "Bringing up 'can0' with bitrate 1000000..."
                      << std::endl;
            return can_interface.up(1000000);
        });

    if (!result) {
        std::cerr << "Failed to set up CAN interface: "
                  << result.error().message << std::endl;
        std::cerr << "Please ensure the 'hycan-daemon' is running and the "
                     "'can0' interface exists."
                  << std::endl;
        return 1;
    }

    std::cout << "'can0' is up. Now sending a test frame every second..."
              << std::endl;

    // 4. Periodically send a test frame
    constexpr can_frame test_frame = {
        .can_id = 0x123,
        .len = 8,
        .data = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED, 0xFA, 0xCE},
    };

    while (keep_running) {
        auto send_result = can_interface.send(test_frame);
        if (send_result) {
            std::cout << "Sent test frame on can0." << std::endl;
        } else {
            std::cerr << "Failed to send frame: " << send_result.error().message
                      << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 5. Bring down the interface before exiting
    std::cout << "Bringing down 'can0'..." << std::endl;
    auto down_result = can_interface.down();
    if (!down_result) {
        std::cerr << "Failed to bring down CAN interface: "
                  << down_result.error().message << std::endl;
    }

    std::cout << "\n=== Example completed ===" << std::endl;

    return 0;
}
