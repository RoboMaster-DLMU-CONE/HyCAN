cmake_minimum_required(VERSION 3.31)

project(HyCAN)

set(CMAKE_CXX_STANDARD 26)
# xtr依赖的fmt库目前使用clang-20构建存在问题，等待修复中
# find_package(xtr)

add_library(HyCAN SHARED)
file(GLOB_RECURSE HYCAN_SOURCES src/*.cxx)
target_sources(HyCAN PUBLIC
        FILE_SET CXX_MODULES
        FILES ${HYCAN_SOURCES}
)
#target_link_libraries(HyCAN PRIVATE xtr::xtr)

add_executable(HyCAN_Example example/example.cpp)
target_link_libraries(HyCAN_Example PRIVATE HyCAN)
