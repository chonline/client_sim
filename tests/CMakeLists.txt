# tests: minimal custom test framework (no GTest dependency).
#
# We use a tiny ASSERT_* / EXPECT_* set implemented in
# tests/wztest/wztest.h. Each test file adds an executable that
# produces a non-zero exit code on failure, and `add_test()` registers
# it with CTest.

add_subdirectory(wztest)
add_subdirectory(test_wz_binary_reader)
add_subdirectory(test_wz_crypto)
add_subdirectory(test_wz_node)
add_subdirectory(test_wz_png_decoder)
add_subdirectory(test_wz_structure)
