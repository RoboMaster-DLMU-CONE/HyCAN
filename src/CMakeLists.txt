add_subdirectory(Daemon)

file(GLOB_RECURSE HYCAN_SOURCES Interface/*.cpp Util/*.cpp)

set(MAX_STD_CAN_ID 2048)

add_library(HyCAN)
target_sources(HyCAN PRIVATE ${HYCAN_SOURCES})

if (NOT TARGET HyCAN::HyCAN)
    add_library(HyCAN::HyCAN ALIAS HyCAN)
endif ()

target_compile_features(HyCAN PUBLIC cxx_std_20)

target_include_directories(HyCAN
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${LIBNL3_INCLUDE_DIR}
)
target_link_libraries(HyCAN
        PUBLIC ${LIBNL3_LIBRARIES} tl::expected HyCAN_UnixSocket
)

target_compile_definitions(HyCAN PUBLIC
        HC_MAX_STD_CAN_ID=${MAX_STD_CAN_ID}
        HYCAN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        HYCAN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        HYCAN_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)