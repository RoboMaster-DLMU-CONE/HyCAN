#include <iostream>
#include <thread>
#include <chrono>

#include "HyCAN/Interface/Daemon.hpp"

using HyCAN::Daemon, HyCAN::DaemonClient, HyCAN::Error;

int main()
{
    constexpr std::string_view test_interface_name = "vcan_test99";
    constexpr std::string_view socket_path = "/tmp/hycan_daemon_test.sock";
    
    std::cout << "--- HyCAN Daemon Test ---" << std::endl;
    std::cout << "INFO: This test requires CAP_NET_ADMIN or root privileges." << std::endl;
    std::cout << "INFO: Testing daemon communication and interface management." << std::endl;

    int test_result_code = EXIT_SUCCESS;

    try
    {
        // Create daemon and start it in a separate thread
        auto daemon = std::make_unique<Daemon>();
        
        std::jthread daemon_thread([&daemon, socket_path]() {
            auto result = daemon->start(std::string(socket_path));
            if (!result)
            {
                std::cerr << "Daemon failed to start: " << result.error().message << std::endl;
            }
        });

        // Give daemon time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Test daemon client operations
        DaemonClient client{std::string(socket_path)};

        // Test 1: Create VCAN interface
        std::cout << "\nTEST 1: Creating VCAN interface '" << test_interface_name << "'..." << std::endl;
        auto create_result = client.create_vcan_interface(test_interface_name);
        if (!create_result)
        {
            std::cerr << "FAIL: " << create_result.error().message << std::endl;
            test_result_code = EXIT_FAILURE;
        }
        else
        {
            std::cout << "PASS: VCAN interface creation succeeded." << std::endl;
        }

        // Test 2: Bring interface up
        std::cout << "\nTEST 2: Bringing interface '" << test_interface_name << "' UP..." << std::endl;
        auto up_result = client.set_interface_up(test_interface_name);
        if (!up_result)
        {
            std::cerr << "FAIL: " << up_result.error().message << std::endl;
            test_result_code = EXIT_FAILURE;
        }
        else
        {
            std::cout << "PASS: Interface UP operation succeeded." << std::endl;
        }

        // Test 3: Bring interface down
        std::cout << "\nTEST 3: Bringing interface '" << test_interface_name << "' DOWN..." << std::endl;
        auto down_result = client.set_interface_down(test_interface_name);
        if (!down_result)
        {
            std::cerr << "FAIL: " << down_result.error().message << std::endl;
            test_result_code = EXIT_FAILURE;
        }
        else
        {
            std::cout << "PASS: Interface DOWN operation succeeded." << std::endl;
        }

        // Clean up
        std::cout << "\nCleaning up daemon..." << std::endl;
        daemon->stop();
        if (daemon_thread.joinable())
        {
            daemon_thread.join();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        test_result_code = EXIT_FAILURE;
    }

    std::cout << "\n--- HyCAN Daemon Test Finished ---" << std::endl;
    return test_result_code;
}