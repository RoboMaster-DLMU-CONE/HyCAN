add_executable(HyCAN_InterfaceTest ${PROJECT_SOURCE_DIR}/tests/InterfaceTest.cpp)
target_link_libraries(HyCAN_InterfaceTest PRIVATE HyCAN)

add_test(
        NAME InterfaceUpDownTest
        COMMAND HyCAN_InterfaceTest
)