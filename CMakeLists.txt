cmake_minimum_required(VERSION 3.20)

project(HyCAN VERSION 0.7.1)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(PROJECT_CXX_STANDARD ${CMAKE_CXX_STANDARD})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_HYCAN_EXAMPLE "Build HyCAN Examples" OFF)
option(BUILD_HYCAN_TEST "Build HyCAN Tests" OFF)
option(TEST_HYCAN_LATENCY "Adding Latency statistics for testing" OFF)

include(HyCANFindTlExpected)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

file(GLOB_RECURSE HYCAN_SOURCES src/*.cpp)
add_library(HyCAN)
target_sources(HyCAN PRIVATE ${HYCAN_SOURCES})

if (NOT TARGET HyCAN::HyCAN)
    add_library(HyCAN::HyCAN ALIAS HyCAN)
endif ()

target_include_directories(HyCAN
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(HyCAN PUBLIC tl::expected)

if (BUILD_HYCAN_EXAMPLE)
    include(HyCANExamples)
endif ()

if (BUILD_HYCAN_TEST)
    enable_testing()
    include(HyCANTests)
endif ()

if (TEST_HYCAN_LATENCY)
    target_compile_definitions(HyCAN PUBLIC HYCAN_LATENCY_TEST)
endif ()

install(TARGETS HyCAN
        EXPORT HyCANTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install sudo rules for CAN interface management without password
install(CODE "
    set(SUDOERS_FILE \"/etc/sudoers.d/can-interface\")
    message(STATUS \"Creating sudo rules file: \${SUDOERS_FILE}\")
    file(WRITE \${SUDOERS_FILE} 
        \"# HyCAN: Allow dialout group to manage CAN interfaces without password\\n\"
        \"%dialout ALL=(ALL) NOPASSWD: /sbin/ip link set can* type can bitrate *\\n\"
        \"%dialout ALL=(ALL) NOPASSWD: /sbin/ip link set can* up\\n\"
        \"%dialout ALL=(ALL) NOPASSWD: /sbin/ip link set can* down\\n\"
        \"%dialout ALL=(ALL) NOPASSWD: /sbin/ip link add * type vcan\\n\"
    )
    execute_process(COMMAND chmod 440 \${SUDOERS_FILE})
    message(STATUS \"Sudo rules installed. Users in 'dialout' group can now manage CAN interfaces without sudo password.\")
    message(STATUS \"Add user to dialout group with: sudo usermod -a -G dialout \$USER\")
")
install(EXPORT HyCANTargets
        FILE HyCANTargets.cmake
        NAMESPACE HyCAN::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/HyCANConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HyCANConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/HyCANConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/HyCANConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/HyCANConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
