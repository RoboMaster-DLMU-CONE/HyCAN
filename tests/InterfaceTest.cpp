#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip> // For printing frame data
#include <optional> // To store received frame
#include <array>  // For std::array
#include <cstring> // For std::memcpy in frame construction if needed

#include <linux/can.h>
#include "HyCAN/Interface/Interface.hpp" // Adjust path if necessary

// Test constants
const std::string TEST_INTERFACE_NAME = "vcan_hytest"; // Unique vcan interface name for this test
constexpr canid_t TEST_CAN_ID = 0x1A3; // A distinct CAN ID for testing
constexpr __u8 TEST_DLC = 8;
const std::array<__u8, TEST_DLC> TEST_DATA = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

// Globals for callback verification
std::atomic g_callback_triggered{false};
std::optional<can_frame> g_received_frame;
// std::mutex g_frame_mutex; // Potentially needed for more complex scenarios

// Test callback function
void test_can_callback(can_frame&& frame)
{
    std::cout << "Callback invoked for CAN ID: 0x" << std::hex << frame.can_id
        << ", DLC: " << std::dec << static_cast<int>(frame.len) << ", Data: ";
    for (int i = 0; i < frame.len; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(frame.data[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    // Store frame and set flag if it's the one we're looking for
    // if (frame.can_id == TEST_CAN_ID) { // Redundant if callback is only for TEST_CAN_ID
    // std::lock_guard<std::mutex> lock(g_frame_mutex);
    g_received_frame = frame;
    g_callback_triggered.store(true, std::memory_order_release);
    // }
}

// Helper to compare can_frames
bool compare_can_frames(const can_frame& f1, const can_frame& f2)
{
    if (f1.can_id != f2.can_id || f1.len != f2.len)
    {
        return false;
    }
    // Compare data only up to f1.len (which should be equal to f2.len)
    for (__u8 i = 0; i < f1.len; ++i)
    {
        if (f1.data[i] != f2.data[i])
        {
            return false;
        }
    }
    return true;
}


int main()
{
    int result_code = EXIT_SUCCESS;
    std::cout << "--- HyCAN Interface Test ---" << std::endl;
    std::cout << "INFO: This test will use virtual CAN interface '" << TEST_INTERFACE_NAME
        << "'." << std::endl;
    std::cout << "INFO: Ensure 'vcan' module is loaded (sudo modprobe vcan)." << std::endl;

    HyCAN::VCANInterface interface(TEST_INTERFACE_NAME);

    // Prepare the frame to be sent
    can_frame frame_to_send{};
    frame_to_send.can_id = TEST_CAN_ID;
    frame_to_send.len = TEST_DLC;
    std::memcpy(frame_to_send.data, TEST_DATA.data(), TEST_DLC);
    g_callback_triggered.store(false, std::memory_order_relaxed);
    g_received_frame.reset();

    (void)interface.tryRegisterCallback<can_frame>({TEST_CAN_ID}, test_can_callback).or_else([&](const auto& e)
    {
        std::cerr << "FAIL: " << e.message;
        result_code = EXIT_FAILURE;
    });

    // --- Test 1: Interface UP and Send/Receive ---
    std::cout << "\nTEST 1: Bringing interface UP and testing send/receive..." << std::endl;
    (void)interface.up().or_else([&](const auto& e)
    {
        std::cerr << "FAIL: " << e.message;
        result_code = EXIT_FAILURE;
    });
    std::cout << "Interface UP command issued. Waiting briefly for setup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Allow time for interface and reaper thread to start

    std::cout << "Sending CAN frame with ID 0x" << std::hex << TEST_CAN_ID << std::dec << "..." << std::endl;

    (void)interface.send(frame_to_send).or_else([&](const auto& e)
    {
        std::cerr << "FAIL: " << e.message;
        result_code = EXIT_FAILURE;
    });

    std::cout << "Waiting for callback (up to 2 seconds)..." << std::endl;
    bool triggered_in_test1 = false;
    for (int i = 0; i < 20; ++i)
    {
        // Poll for up to 2 seconds (20 * 100ms)
        if (g_callback_triggered.load(std::memory_order_acquire))
        {
            triggered_in_test1 = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!triggered_in_test1)
    {
        std::cerr << "FAIL: Callback was not triggered after sending message." << std::endl;
        result_code = EXIT_FAILURE;
    }
    else
    {
        // std::lock_guard<std::mutex> lock(g_frame_mutex); // If needed
        if (!g_received_frame)
        {
            std::cerr << "FAIL: Callback triggered but no frame was stored by the callback." << std::endl;
            result_code = EXIT_FAILURE;
        }
        else if (!compare_can_frames(*g_received_frame, frame_to_send))
        {
            std::cerr << "FAIL: Received CAN frame does not match the sent frame." << std::endl;
            std::cerr << "  Expected ID: 0x" << std::hex << frame_to_send.can_id << ", DLC: " << std::dec << static_cast
                <int>(frame_to_send.len) << std::endl;
            std::cerr << "  Received ID: 0x" << std::hex << g_received_frame->can_id << ", DLC: " << std::dec <<
                static_cast<int>(g_received_frame->len) << std::endl;
            // Could add data dump here if needed
            result_code = EXIT_FAILURE;
        }
        else
        {
            std::cout << "PASS: Callback triggered and received frame matches sent frame." << std::endl;
        }
    }

    // --- Test 2: Interface DOWN and Verify No More Callbacks ---
    std::cout << "\nTEST 2: Bringing interface DOWN and verifying no messages are received..." << std::endl;
    (void)interface.down().or_else([&](const auto& e)
    {
        std::cerr << "FAIL: " << e.message;
        result_code = EXIT_FAILURE;
    });
    std::cout << "Interface DOWN command issued. Waiting briefly for teardown..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Allow time for reaper thread to stop

    g_callback_triggered.store(false, std::memory_order_relaxed);
    g_received_frame.reset();

    std::cout << "Sending CAN frame again (interface should be down)..." << std::endl;
    // The send operation might log an error if the underlying socket is closed or interface is down.
    // The primary check is that the reaper thread (and thus callback) is no longer active.
    (void)interface.send(frame_to_send).or_else([&](const auto& e)
    {
        std::cerr << "Sending CAN Message failed." << e.message << std::endl;
    });

    std::cout << "Waiting to see if callback is (incorrectly) triggered (1 second)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait to ensure no callback occurs

    if (g_callback_triggered.load(std::memory_order_acquire))
    {
        std::cerr << "FAIL: Callback was triggered even after interface.down() was called." << std::endl;
        if (g_received_frame)
        {
            std::cerr << "  (Received ID: 0x" << std::hex << g_received_frame->can_id << ")" << std::dec << std::endl;
        }
        result_code = EXIT_FAILURE;
    }
    else
    {
        std::cout << "PASS: Callback was not triggered after interface.down(), as expected." << std::endl;
    }

    std::cout << "\n--- HyCAN Interface Test Finished ---" << std::endl;
    if (result_code == EXIT_SUCCESS)
    {
        std::cout << "All tests passed successfully." << std::endl;
    }
    else
    {
        std::cout << "One or more tests FAILED." << std::endl;
    }

    return result_code;
}
