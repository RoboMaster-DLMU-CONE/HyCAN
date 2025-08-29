add_executable(HyCAN_NetlinkTest ${PROJECT_SOURCE_DIR}/tests/NetlinkTest.cpp)
add_executable(HyCAN_InterfaceTest ${PROJECT_SOURCE_DIR}/tests/InterfaceTest.cpp)
add_executable(HyCAN_InterfaceStressTest ${PROJECT_SOURCE_DIR}/tests/InterfaceStressTest.cpp)
add_executable(HyCAN_DaemonTest ${PROJECT_SOURCE_DIR}/tests/DaemonTest.cpp)

target_link_libraries(HyCAN_NetlinkTest PRIVATE HyCAN)
target_link_libraries(HyCAN_InterfaceTest PRIVATE HyCAN)
target_link_libraries(HyCAN_InterfaceStressTest PRIVATE HyCAN)
target_link_libraries(HyCAN_DaemonTest PRIVATE HyCAN)

add_test(
        NAME NetlinkUpDownTest
        COMMAND HyCAN_NetlinkTest
)

add_test(
        NAME InterfaceTest
        COMMAND HyCAN_InterfaceTest
)

add_test(
        NAME InterfaceStressTest
        COMMAND HyCAN_InterfaceStressTest
)

add_test(
        NAME DaemonTest
        COMMAND HyCAN_DaemonTest
)