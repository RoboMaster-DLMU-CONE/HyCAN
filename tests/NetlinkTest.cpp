#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "HyCAN/Interface/Interface.hpp"
#include "HyCAN/Interface/Netlink.hpp"
using HyCAN::Netlink;

// helper: returns true if interface name resolves
static bool interface_exists(const std::string_view& name)
{
    return if_nametoindex(name.data()) != 0;
}

// helper: returns true if interface is flagged UP
static bool is_interface_up(const std::string_view& name)
{
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;
    ifreq ifr{};
    const auto name_len = std::min(name.size(), static_cast<size_t>(IFNAMSIZ - 1));
    std::memcpy(ifr.ifr_name, name.data(), name_len);
    ifr.ifr_name[name_len] = '\0';
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
    {
        close(fd);
        return false;
    }
    close(fd);
    return (ifr.ifr_flags & IFF_UP) != 0;
}

int main()
{
    constexpr std::string_view test_interface_name = "vcan_test0";
    int test_result_code = EXIT_SUCCESS;

    std::cout << "--- HyCAN Netlink Management Test ---" << std::endl;
    std::cout << "INFO: This test requires CAP_NET_ADMIN or root privileges." << std::endl;
    std::cout << "INFO: Using test interface: " << test_interface_name << std::endl;

    auto& netlink = Netlink::instance();

    // --- Test 1: Set Virtual Interface UP ---
    std::cout << "\nTEST 1: Bringing UP virtual interface '" << test_interface_name << "'..." << std::endl;
    (void)netlink.set(test_interface_name, true).or_else([&](const auto& e)
    {
        std::cerr << "FAIL: " << e.message << std::endl;
        test_result_code = EXIT_FAILURE;
    });

    std::cout << "PASS: Netlink::set(up) for '" << test_interface_name << "' succeeded." << std::endl;

    // existence & state check after up()
    if (!interface_exists(test_interface_name))
    {
        std::cerr << "FAIL: Interface '" << test_interface_name << "' does not exist after up()" << std::endl;
        test_result_code = EXIT_FAILURE;
    }
    else if (!is_interface_up(test_interface_name))
    {
        std::cerr << "FAIL: Interface '" << test_interface_name << "' is not UP after up()" << std::endl;
        test_result_code = EXIT_FAILURE;
    }

    // --- Test 2: Set Interface DOWN ---
    std::cout << "\nTEST 2: Bringing DOWN interface '" << test_interface_name << "'..." << std::endl;
    (void)netlink.set(test_interface_name, false).or_else([&](const auto& e)
    {
        std::cerr << "FAIL: " << e.message << std::endl;
        test_result_code = EXIT_FAILURE;
    });
    std::cout << "PASS: Netlink::set(down) for '" << test_interface_name << "' succeeded." << std::endl;

    // existence & state check after down()
    if (!interface_exists(test_interface_name))
    {
        std::cerr << "FAIL: Interface '" << test_interface_name << "' does not exist after down()" << std::endl;
        test_result_code = EXIT_FAILURE;
    }
    else if (is_interface_up(test_interface_name))
    {
        std::cerr << "FAIL: Interface '" << test_interface_name << "' is still UP after down()" << std::endl;
        test_result_code = EXIT_FAILURE;
    }

    std::cout << "\n--- HyCAN Netlink Management Test Finished ---" << std::endl;
    return test_result_code;
}
