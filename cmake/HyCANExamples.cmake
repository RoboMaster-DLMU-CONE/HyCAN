add_executable(HyCAN_Example_SendCallback ${PROJECT_SOURCE_DIR}/example/send&callback.cpp)
target_link_libraries(HyCAN_Example_SendCallback PRIVATE HyCAN)

add_executable(HyCAN_Example_RealCAN ${PROJECT_SOURCE_DIR}/example/real_can_example.cpp)
target_link_libraries(HyCAN_Example_RealCAN PRIVATE HyCAN)
