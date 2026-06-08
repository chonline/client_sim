add_executable(test_wz_node main.cpp)
target_link_libraries(test_wz_node PRIVATE wzlib wztest)
add_test(NAME test_wz_node COMMAND test_wz_node)
