cmake_minimum_required(VERSION 3.31)
project(HyCAN)

set(CMAKE_CXX_STANDARD 26)

add_library(HyCAN SHARED)
file(GLOB_RECURSE HYCAN_SOURCES src/*.cxx)
target_sources(HyCAN PUBLIC
        FILE_SET CXX_MODULES
        FILES ${HYCAN_SOURCES}
)
add_executable(HyCAN_Example example/example.cpp)

target_include_directories(HyCAN_Example PRIVATE src)
target_link_libraries(HyCAN_Example PRIVATE HyCAN)
