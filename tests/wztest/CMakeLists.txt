# wztest is a header-only library; this file exists for CMake organization.
# We add an INTERFACE library so the test executables can depend on it.
# The include base is the tests/ directory so tests can do
# `#include "wztest/wztest.h"`.

add_library(wztest INTERFACE)
target_include_directories(wztest INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
