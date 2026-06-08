# test_wz_structure: wzlib-level tests for WzStructure / cross-WZ path
# resolution / WzPng decodeRgba byte order.
#
# Real-WZ-fixture tests will be added in a follow-up once the user
# has placed a KMS sample under tests/fixtures/wz/. See
# tests/fixtures/wz/README.md.

add_executable(test_wz_structure main.cpp)
target_link_libraries(test_wz_structure
    PRIVATE
        wzlib
        wztest
)

add_test(NAME test_wz_structure COMMAND test_wz_structure)
