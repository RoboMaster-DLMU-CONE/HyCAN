cmake_minimum_required(VERSION 3.31)

project(HyCAN)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 23)
option(BUILD_HYCAN_EXAMPLE "Build HyCAN Examples" ON)
option(BUILD_HYCAN_TEST "Build HyCAN Tests" ON)

find_package(xtr REQUIRED)
find_package(libnl REQUIRED)

file(GLOB_RECURSE HYCAN_SOURCES src/*.cpp)
add_library(HyCAN SHARED)
target_sources(HyCAN PRIVATE ${HYCAN_SOURCES})
target_include_directories(HyCAN PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(HyCAN PUBLIC xtr::xtr libnl::libnl)

if (BUILD_HYCAN_EXAMPLE)
    include(HyCANExamples)
endif ()

if (BUILD_HYCAN_TEST)
    enable_testing()
    include(HyCANTests)
endif ()