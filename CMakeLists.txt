cmake_minimum_required(VERSION 3.31)

project(HyCAN)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 23)
option(BUILD_HYCAN_EXAMPLE "Build HyCAN Examples" ON)
option(BUILD_HYCAN_TEST "Build HyCAN Tests" ON)

add_library(HyCAN SHARED)
file(GLOB_RECURSE HYCAN_SOURCES src/*.cxxm)
target_sources(HyCAN PUBLIC
        FILE_SET CXX_MODULES
        FILES ${HYCAN_SOURCES}
)
# xtr依赖的fmt库目前使用clang-20构建存在问题，等待修复中
# find_package(xtr REQUIRED)
#target_link_libraries(HyCAN PRIVATE xtr::xtr)

if (BUILD_HYCAN_EXAMPLE)
    include(HyCANExamples)
endif ()

if (BUILD_HYCAN_TEST)
    enable_testing()
    include(HyCANTests)
endif ()