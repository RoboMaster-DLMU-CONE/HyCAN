add_executable(HyCAN_Daemon
        Daemon.cpp
)

target_include_directories(HyCAN_Daemon PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(HyCAN_Daemon PRIVATE ipc)
