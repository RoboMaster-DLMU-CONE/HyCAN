add_subdirectory(UnixSocket)

add_executable(HyCAN_Daemon
        Daemon.cpp
        VCAN.cpp
        main.cpp
)

target_include_directories(HyCAN_Daemon PRIVATE ${PROJECT_SOURCE_DIR}/include ${LIBNL3_INCLUDE_DIR})

target_link_libraries(HyCAN_Daemon PRIVATE tl::expected ${LIBNL3_LIBRARIES} HyCAN_UnixSocket)

install(TARGETS HyCAN_Daemon
        RUNTIME DESTINATION bin
)

configure_file(
        systemed/hycan-daemon.service.in
        ${CMAKE_CURRENT_BINARY_DIR}/systemed/hycan-daemon.service
        @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/systemed/hycan-daemon.service
        DESTINATION lib/systemd/system
)

install(
        CODE "execute_process(COMMAND sudo systemctl enable --now hycan-daemon.service)"
        CODE "execute_process(COMMAND sudo systemctl restart hycan-daemon.service)"
)