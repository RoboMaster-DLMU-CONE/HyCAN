#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <memory> // For std::unique_ptr
#include <iomanip> // For std::setw, std::setfill
#include <numeric> // For std::iota

#include <linux/can.h>


#include "hycan/Interface/Interface.hpp" // Adjust path if necessary
#include "hycan/Interface/Logger.hpp"    // For sink, if direct logging is needed

// --- Test Configuration ---
constexpr int NUM_INTERFACES = 10;
constexpr int NUM_SENDER_THREADS = 10;
constexpr int TEST_DURATION_SECONDS = 10; // How long the sending phase lasts
constexpr unsigned int SEND_INTERVAL_MICROSECONDS = 1; // Delay between sends per thread (0 for max effort)
constexpr canid_t BASE_CAN_ID = 0x200; // Starting CAN ID for interfaces
const std::string VCAN_BASENAME = "vcan_stress_";

// --- Global Atomics for Tracking ---
std::array<std::atomic<uint64_t>, NUM_INTERFACES> g_received_message_counts{};
std::atomic g_stop_sending_flag{false};
std::atomic<uint64_t> g_total_sent_attempts{0};

// --- Callback Function ---
// This callback is invoked by the Reaper thread when a message for the registered CAN ID is received.
void stress_test_callback(can_frame&& frame, const int interface_index, canid_t expected_can_id)
{
    // frame.can_id should match expected_can_id due to Reaper's dispatching
    if (interface_index >= 0 && static_cast<size_t>(interface_index) < g_received_message_counts.size())
    {
        g_received_message_counts[interface_index].fetch_add(1, std::memory_order_relaxed);
    }

    // Minimal logging to avoid performance impact during stress.
    // Can be enabled for debugging.
    /*
    static std::atomic<uint64_t> s_callback_invocations{0};
    if (s_callback_invocations.fetch_add(1, std::memory_order_relaxed) % 10000 == 0) { // Log every 10000th invocation
        std::cout << "Callback on iface " << interface_index
                  << " (ID 0x" << std::hex << expected_can_id << std::dec
                  << "): Total calls ~" << s_callback_invocations.load() << std::endl;
    }
    */
}

// --- Sender Thread Worker Function ---
void sender_worker_fn(
    const int thread_id,
    const canid_t base_can_id)
{
    std::cout << "Sender thread " << thread_id << " started." << std::endl;
    can_frame frame_to_send{};
    frame_to_send.len = 8;

    std::array<std::unique_ptr<HyCAN::Sender>, NUM_INTERFACES> senders;
    std::array<std::string, NUM_INTERFACES> names;
    for (int i = 0; i < NUM_INTERFACES; i++)
    {
        names[i] = format("vcan_stress_{}", i);
        senders[i] = std::make_unique<HyCAN::Sender>(HyCAN::Sender(names[i]));
    }
    uint64_t messages_sent_by_this_thread = 0;
    int interface_rr_index = thread_id % NUM_INTERFACES;

    while (!g_stop_sending_flag.load(std::memory_order_acquire))
    {
        frame_to_send.can_id = base_can_id + interface_rr_index;

#ifdef HYCAN_LATENCY_TEST
        auto send_time = std::chrono::high_resolution_clock::now();
        uint64_t send_timestamp_ns_count = std::chrono::duration_cast<std::chrono::nanoseconds>(
            send_time.time_since_epoch()).count();
        std::memcpy(frame_to_send.data, &send_timestamp_ns_count, sizeof(send_timestamp_ns_count));
#else
        for (__u8 i = 0; i < frame_to_send.len; ++i)
        {
            frame_to_send.data[i] = static_cast<__u8>(thread_id + i);
        }
#endif

        if (senders[interface_rr_index])
        {
            senders[interface_rr_index]->send(frame_to_send);
            messages_sent_by_this_thread++;
        }

        interface_rr_index = (interface_rr_index + 1) % NUM_INTERFACES;

        if constexpr (SEND_INTERVAL_MICROSECONDS > 0)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(SEND_INTERVAL_MICROSECONDS));
        }
    }
    g_total_sent_attempts.fetch_add(messages_sent_by_this_thread, std::memory_order_relaxed);
    std::cout << "Sender thread " << thread_id << " finished. Attempted to send " << messages_sent_by_this_thread <<
        " messages." << std::endl;
}

int main()
{
    std::cout << "--- HyCAN Interface Stress Test ---" << std::endl;
    std::cout << "INFO: This test requires CAP_NET_ADMIN or root privileges." << std::endl;
    std::cout << "INFO: Ensure 'vcan' module is loaded (sudo modprobe vcan)." << std::endl;
    std::cout << "Config: " << NUM_INTERFACES << " interfaces, "
        << NUM_SENDER_THREADS << " sender threads, "
        << TEST_DURATION_SECONDS << "s duration." << std::endl;
    std::cout << "Messages sent with interval: " << SEND_INTERVAL_MICROSECONDS << " us." << std::endl;

    for (int i = 0; i < NUM_INTERFACES; ++i)
    {
        g_received_message_counts[i].store(0, std::memory_order_relaxed);
    }

    std::vector<std::unique_ptr<HyCAN::Interface>> hycan_interfaces;
    hycan_interfaces.reserve(NUM_INTERFACES);

    // 1. Create and Setup Interfaces
    std::cout << "\n--- Phase 1: Initializing " << NUM_INTERFACES << " Interfaces ---" << std::endl;
    for (int i = 0; i < NUM_INTERFACES; ++i)
    {
        std::string if_name = VCAN_BASENAME + std::to_string(i);
        canid_t target_can_id = BASE_CAN_ID + i;

        if (target_can_id >= 2048)
        {
            // Reaper.hpp has funcs[2048]
            std::cerr << "ERROR: CAN ID 0x" << std::hex << target_can_id << std::dec
                << " exceeds Reaper's max callback ID limit. Adjust BASE_CAN_ID or NUM_INTERFACES." << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Creating interface: " << if_name << " listening to CAN ID 0x" << std::hex << target_can_id <<
            std::dec << std::endl;
        try
        {
            auto interface_ptr = std::make_unique<HyCAN::Interface>(if_name);

            // Register callback BEFORE calling up()
            auto callback_for_interface =
                [i, target_can_id](can_frame&& frame)
            {
                stress_test_callback(std::move(frame), i, target_can_id);
            };

            if (auto reg_status = interface_ptr->registerCallback(target_can_id, callback_for_interface); !reg_status)
            {
                std::cerr << "FAIL: Callback registration failed for " << if_name << " and ID 0x" << std::hex <<
                    target_can_id << std::dec << ": " << reg_status.error() << std::endl;
                // Depending on desired strictness, could exit here or just skip this interface
                // For now, we'll try to continue, but this interface won't receive messages.
                hycan_interfaces.emplace_back(nullptr); // Placeholder for failed interface
                continue;
            }
            std::cout << "Callback registered for " << if_name << " on ID 0x" << std::hex << target_can_id << std::dec
                << std::endl;
            hycan_interfaces.push_back(std::move(interface_ptr));
        }
        catch (const std::exception& e)
        {
            std::cerr << "ERROR: Exception during Interface creation for " << if_name << ": " << e.what() << std::endl;
            // This might happen if XTR_LOGL(fatal, ...) is called, which exits.
            // If it doesn't exit, we add a nullptr.
            hycan_interfaces.emplace_back(nullptr);
            return EXIT_FAILURE; // If any interface fails to init, stop the test.
        }
    }

    // Bring all successfully created interfaces up
    std::cout << "\nBringing interfaces UP..." << std::endl;
    for (int i = 0; i < NUM_INTERFACES; ++i)
    {
        if (hycan_interfaces[i])
        {
            std::string if_name = VCAN_BASENAME + std::to_string(i);
            std::cout << "Bringing UP " << if_name << "..." << std::endl;
            try
            {
                hycan_interfaces[i]->up();
            }
            catch (const std::exception& e)
            {
                std::cerr << "ERROR: Exception during Interface::up() for " << if_name << ": " << e.what() << std::endl;
                // Mark as unusable for sending
                hycan_interfaces[i].reset(); // Release/destroy this interface
            }
        }
    }

    std::cout << "Waiting a moment for interfaces to settle..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Give time for reaper threads to start, etc.

    // 2. Start Sender Threads
    std::cout << "\n--- Phase 2: Starting " << NUM_SENDER_THREADS << " Sender Threads ---" << std::endl;
    std::vector<std::jthread> sender_threads;
    sender_threads.reserve(NUM_SENDER_THREADS);
    g_stop_sending_flag.store(false, std::memory_order_release);

    for (int i = 0; i < NUM_SENDER_THREADS; ++i)
    {
        sender_threads.emplace_back(sender_worker_fn, i, BASE_CAN_ID);
    }

    // 3. Run Test for Duration
    std::cout << "\nStress test running for " << TEST_DURATION_SECONDS << " seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(TEST_DURATION_SECONDS));

    // 4. Stop Senders
    std::cout << "\n--- Phase 3: Stopping Sender Threads ---" << std::endl;
    g_stop_sending_flag.store(true, std::memory_order_release);
    for (auto& t : sender_threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    std::cout << "All sender threads joined." << std::endl;

    // 5. Teardown Interfaces
    std::cout << "\n--- Phase 4: Bringing Interfaces DOWN ---" << std::endl;
    for (int i = 0; i < NUM_INTERFACES; ++i)
    {
        if (hycan_interfaces[i])
        {
            std::string if_name = VCAN_BASENAME + std::to_string(i);
            std::cout << "Bringing DOWN " << if_name << "..." << std::endl;
            try
            {
                hycan_interfaces[i]->down();
            }
            catch (const std::exception& e)
            {
                std::cerr << "ERROR: Exception during Interface::down() for " << if_name << ": " << e.what() <<
                    std::endl;
            }
        }
    }
    // Interfaces unique_ptrs will be destroyed upon leaving scope.

    // 6. Report Results
    std::cout << "\n--- Stress Test Results ---" << std::endl;
    std::cout << "Total messages send attempts by all threads: " << g_total_sent_attempts.load() << std::endl;
    uint64_t total_received_across_all_interfaces = 0;
    for (int i = 0; i < NUM_INTERFACES; ++i)
    {
        std::string if_name = VCAN_BASENAME + std::to_string(i);
        const canid_t target_can_id = BASE_CAN_ID + i;
        const uint64_t count = g_received_message_counts[i].load();
        std::cout << "Interface " << if_name << " (ID 0x" << std::hex << target_can_id << std::dec << "): Received " <<
            count << " messages." << std::endl;
        total_received_across_all_interfaces += count;
    }
    std::cout << "Total messages received across all interfaces: " << total_received_across_all_interfaces << std::endl;
#ifdef HYCAN_LATENCY_TEST
    std::cout << "\n--- Latency Test Results ---" << std::endl;
    uint64_t overall_latency_messages = 0;
    double overall_weighted_latency_sum_us = 0.0;

    for (int i = 0; i < NUM_INTERFACES; ++i)
    {
        if (hycan_interfaces[i])
        {
            // 确保接口对象有效
            std::string if_name = VCAN_BASENAME + std::to_string(i);

            if (const HyCAN::Reaper::LatencyStats stats = hycan_interfaces[i]->get_reaper_latency_stats(); stats.
                message_count > 0)
            {
                std::cout << "Interface " << if_name << ": Avg Latency "
                    << std::fixed << std::setprecision(2) << stats.average_latency_us << " us "
                    << "(" << stats.message_count << " msgs)" << std::endl;
                overall_latency_messages += stats.message_count;
                overall_weighted_latency_sum_us += stats.average_latency_us * static_cast<double>(stats.message_count);
            }
            else
            {
                std::cout << "Interface " << if_name << ": No latency messages processed." << std::endl;
            }
        }
    }
    if (overall_latency_messages > 0)
    {
        const double final_overall_avg_latency_us = overall_weighted_latency_sum_us / static_cast<double>(
            overall_latency_messages);
        std::cout << "Overall Average Latency: "
            << std::fixed << std::setprecision(2) << final_overall_avg_latency_us << " us "
            << "(" << overall_latency_messages << " total msgs)" << std::endl;
    }
    else
    {
        std::cout << "No latency messages processed overall for any interface." << std::endl;
    }
#endif

    std::cout << "\n--- HyCAN Interface Stress Test Finished ---" << std::endl;
    return EXIT_SUCCESS;
}
