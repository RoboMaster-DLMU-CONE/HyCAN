#include <iostream>
#include <string_view>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>

import HyCAN.Interface;

// Helper function to check if a network interface exists
bool check_interface_exists(const std::string_view interface_name)
{
    errno = 0; // Clear errno before the call
    const unsigned int if_idx = if_nametoindex(interface_name.data());
    // if_nametoindex returns 0 if the interface is not found or an error occurs.
    // If an error occurs, errno is set. If not found, errno might be ENODEV or not set.
    return if_idx != 0;
}

int main()
{
    constexpr std::string_view test_interface_name = "vcan_test0";
    int test_result_code = EXIT_SUCCESS;

    std::cout << "--- HyCAN Interface Management Test ---" << std::endl;
    std::cout <<
        "INFO: This test requires CAP_NET_ADMIN capability or root privileges to manipulate network interfaces." <<
        std::endl;
    std::cout << "INFO: Using test interface: " << test_interface_name << std::endl;

    // --- Pre-Test Cleanup Attempt ---
    // Try to bring down the interface if it exists from a previous run to ensure a cleaner state.
    if (check_interface_exists(test_interface_name))
    {
        std::cout << "INFO: Test interface '" << test_interface_name << "' detected. Attempting pre-test takedown." <<
            std::endl;
        if (auto cleanup_down_result = set_interface_down(test_interface_name); !cleanup_down_result)
        {
            std::cerr << "WARN: Pre-test cleanup of '" << test_interface_name << "' failed: " << cleanup_down_result.
                error() << std::endl;
        }
        else
        {
            std::cout << "INFO: Pre-test cleanup of '" << test_interface_name << "' successful." << std::endl;
        }
    }

    // --- Test 1: Set Virtual Interface UP ---
    std::cout << "\nTEST 1: Setting up virtual interface '" << test_interface_name << "'..." << std::endl;
    auto up_result = set_interface_up<CANInterfaceType::Virtual>(
        test_interface_name);

    if (!up_result)
    {
        std::cerr << "FAIL: set_interface_up<Virtual> for '" << test_interface_name << "' failed." << std::endl;
        std::cerr << "      Error: " << up_result.error() << std::endl;
        test_result_code = EXIT_FAILURE;
    }
    else
    {
        std::cout << "PASS: set_interface_up<Virtual> for '" << test_interface_name << "' reported success." <<
            std::endl;
        if (!check_interface_exists(test_interface_name))
        {
            std::cerr << "FAIL: Interface '" << test_interface_name <<
                "' does NOT exist after set_interface_up reported success." << std::endl;
            test_result_code = EXIT_FAILURE;
        }
        else
        {
            std::cout << "PASS: Interface '" << test_interface_name << "' confirmed to exist after set_interface_up." <<
                std::endl;
            // Note: To confirm it's truly "UP", one would need to check IFF_UP flag via ioctl(SIOCGIFFLAGS).
            // This test primarily checks creation and the function's return value.
        }
    }

    // --- Test 2: Set Interface DOWN ---
    // This test is more meaningful if the interface was successfully brought up or at least attempted.
    if (up_result)
    {
        // Proceed if the set_interface_up call itself didn't return an error.
        std::cout << "\nTEST 2: Setting down interface '" << test_interface_name << "'..." << std::endl;

        if (auto down_result = set_interface_down(test_interface_name); !down_result)
        {
            std::cerr << "FAIL: set_interface_down for '" << test_interface_name << "' failed." << std::endl;
            std::cerr << "      Error: " << down_result.error() << std::endl;
            test_result_code = EXIT_FAILURE;
        }
        else
        {
            std::cout << "PASS: set_interface_down for '" << test_interface_name << "' reported success." << std::endl;
            // Note: The interface will still exist after being set "down" (IFF_UP cleared).
            // `create_vcan_interface_if_not_exists` creates it, `set_interface_down` only changes flags.
            // A RTM_DELLINK operation would be needed to remove it.
            if (check_interface_exists(test_interface_name))
            {
                std::cout << "INFO: Interface '" << test_interface_name <<
                    "' still exists (as expected) after set_interface_down." << std::endl;
            }
            else
            {
                std::cerr << "WARN: Interface '" << test_interface_name <<
                    "' unexpectedly does not exist after set_interface_down." << std::endl;
            }
        }
    }
    else
    {
        std::cout << "\nINFO: Skipping Test 2 (set_interface_down) because set_interface_up failed to report success."
            << std::endl;
    }

    std::cout << "\n--- HyCAN Interface Management Test Finished ---" << std::endl;
    if (test_result_code == EXIT_SUCCESS)
    {
        std::cout << "RESULT: All tests passed." << std::endl;
    }
    else
    {
        std::cout << "RESULT: One or more tests FAILED." << std::endl;
    }

    return test_result_code;
}
