add_executable(test_wz_crypto main.cpp)
target_link_libraries(test_wz_crypto PRIVATE wzlib wztest)
add_test(NAME test_wz_crypto COMMAND test_wz_crypto)
