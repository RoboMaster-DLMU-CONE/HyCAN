cmake_minimum_required(VERSION 3.20)

project(HyCAN VERSION 0.9.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(PROJECT_CXX_STANDARD ${CMAKE_CXX_STANDARD})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_HYCAN_EXAMPLE "Build HyCAN Examples" OFF)
option(BUILD_HYCAN_TEST "Build HyCAN Tests" OFF)
option(BUILD_HYCAN_PACKAGE "Build HyCAN Packages" OFF)
option(TEST_HYCAN_LATENCY "Adding Latency statistics for testing" OFF)

include(FetchContent)
include(HyCANFindLibnl3)
include(HyCANFindTlExpected)
include(HyCANFindNontypeFunctional)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_subdirectory(src)

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

include(HyCANInstall)

if (BUILD_HYCAN_PACKAGE)
  include(HyCANPackaging)
endif ()
