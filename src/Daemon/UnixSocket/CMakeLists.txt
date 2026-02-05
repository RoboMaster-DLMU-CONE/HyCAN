add_library(HyCAN_UnixSocket
    UnixSocket.cpp
)

target_include_directories(HyCAN_UnixSocket PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(HyCAN_UnixSocket PUBLIC cxx_std_20)