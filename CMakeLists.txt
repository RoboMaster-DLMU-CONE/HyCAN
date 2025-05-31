cmake_minimum_required(VERSION 3.27)

project(HyCAN VERSION 0.3.2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 23)

option(BUILD_HYCAN_EXAMPLE "Build HyCAN Examples" ON)
option(BUILD_HYCAN_TEST "Build HyCAN Tests" ON)
option(TEST_HYCAN_LATENCY "Adding Latency statistics for testing" ON)

find_package(xtr REQUIRED)
find_package(libnl REQUIRED)

file(GLOB_RECURSE HYCAN_SOURCES src/*.cpp)
add_library(HyCAN)
target_sources(HyCAN PRIVATE ${HYCAN_SOURCES})
target_include_directories(HyCAN PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(HyCAN PUBLIC xtr::xtr libnl::libnl)

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

include(GNUInstallDirs)
install(TARGETS HyCAN
        EXPORT HyCANTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT HyCANTargets
        FILE HyCANTargets.cmake
        NAMESPACE HyCAN::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HyCAN
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        HyCANConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HyCANConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/HyCANConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HyCAN
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/HyCANConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/HyCANConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HyCAN
)