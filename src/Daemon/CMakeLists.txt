add_executable(HyCAN_Daemon
        Daemon.cpp
        VCAN.cpp
)

target_include_directories(HyCAN_Daemon PRIVATE ${PROJECT_SOURCE_DIR}/include ${LIBNL3_INCLUDE_DIR})

target_link_libraries(HyCAN_Daemon PRIVATE cpp-ipc::ipc ${LIBNL3_LIBRARIES})

install(TARGETS HyCAN_Daemon
        RUNTIME DESTINATION bin
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/hycan-daemon.service.in
        ${CMAKE_CURRENT_BINARY_DIR}/hycan-daemon.service
        @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hycan-daemon.service
        DESTINATION lib/systemd/system
)

install(
        CODE "execute_process(COMMAND sudo systemctl enable hycan-daemon.service)"
        CODE "execute_process(COMMAND sudo systemctl restart hycan-daemon.service)"
)