add_executable(HyCAN_NetlinkTest ${PROJECT_SOURCE_DIR}/tests/NetlinkTest.cpp)
target_link_libraries(HyCAN_NetlinkTest PRIVATE HyCAN fmt::fmt)

add_test(
        NAME NetlinkUpDownTest
        COMMAND HyCAN_NetlinkTest
)