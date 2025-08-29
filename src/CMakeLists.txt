add_subdirectory(Daemon)

file(GLOB_RECURSE HYCAN_SOURCES Interface/*.cpp)
add_library(HyCAN)
target_sources(HyCAN PRIVATE ${HYCAN_SOURCES})

if (NOT TARGET HyCAN::HyCAN)
    add_library(HyCAN::HyCAN ALIAS HyCAN)
endif ()

target_include_directories(HyCAN
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${LIBNL3_INCLUDE_DIR}
)
target_link_libraries(HyCAN
        PUBLIC ${LIBNL3_LIBRARIES} tl::expected
        PRIVATE cpp-ipc::ipc
)