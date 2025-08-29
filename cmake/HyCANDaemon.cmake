# HyCAN Daemon configuration

# Add daemon executable
add_executable(hycan_daemon ${PROJECT_SOURCE_DIR}/daemon/hycan_daemon.cpp)
target_link_libraries(hycan_daemon PRIVATE HyCAN)

# Install daemon executable
install(TARGETS hycan_daemon
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create systemd service file
set(DAEMON_SOCKET_PATH "/var/run/hycan_daemon.sock")
set(DAEMON_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/hycan_daemon")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/systemd/hycan-daemon.service.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hycan-daemon.service"
    @ONLY
)

# Install systemd service file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/hycan-daemon.service"
        DESTINATION /etc/systemd/system/
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# Post-install script to enable and start the service
install(CODE "
    if(NOT DEFINED CMAKE_CROSSCOMPILING OR NOT CMAKE_CROSSCOMPILING)
        execute_process(COMMAND systemctl daemon-reload)
        execute_process(COMMAND systemctl enable hycan-daemon.service)
        execute_process(COMMAND systemctl start hycan-daemon.service)
        message(STATUS \"HyCAN daemon service enabled and started\")
    endif()
")