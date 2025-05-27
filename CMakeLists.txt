cmake_minimum_required(VERSION 3.31)

project(HyCAN)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 23)
option(BUILD_HYCAN_EXAMPLE "Build HyCAN Examples" ON)
option(BUILD_HYCAN_TEST "Build HyCAN Tests" ON)

find_package(xtr REQUIRED)

add_library(HyCAN SHARED)
file(GLOB_RECURSE HYCAN_SOURCES src/*.cppm)
target_sources(HyCAN PUBLIC
        FILE_SET CXX_MODULES
        FILES ${HYCAN_SOURCES}
)
target_link_libraries(HyCAN PRIVATE xtr::xtr)

if (BUILD_HYCAN_EXAMPLE)
    include(HyCANExamples)
endif ()

if (BUILD_HYCAN_TEST)
    enable_testing()
    include(HyCANTests)
endif ()