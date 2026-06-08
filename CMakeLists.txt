cmake_minimum_required(VERSION 3.21)

project(client_sim
    VERSION 0.1.0
    DESCRIPTION "MapleStory GM Client - C++/Qt Cross-Platform Port"
    LANGUAGES CXX
)

# ----------------------------------------------------------------------------
# Project-wide settings
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if user didn't specify
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ----------------------------------------------------------------------------
# Output directories
# ----------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ----------------------------------------------------------------------------
# Options
# ----------------------------------------------------------------------------
option(CLIENT_SIM_BUILD_TESTS "Build unit tests" ON)
option(CLIENT_SIM_BUILD_TOOLS "Build CLI tools (e.g. wzdump)" ON)
option(CLIENT_SIM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(CLIENT_SIM_USE_ASAN "Enable AddressSanitizer" OFF)
option(CLIENT_SIM_USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# ----------------------------------------------------------------------------
# Compiler flags
# ----------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    if(CLIENT_SIM_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter       # Common with virtual functions
        -Wno-deprecated-declarations # Some libs use deprecated APIs
    )
    if(CLIENT_SIM_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()

    if(CLIENT_SIM_USE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
    if(CLIENT_SIM_USE_UBSAN)
        add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined)
    endif()
endif()

# ----------------------------------------------------------------------------
# Find dependencies
# ----------------------------------------------------------------------------
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Qt 6 (optional for M1, required for M2+)
find_package(Qt6 6.5 QUIET)

# ----------------------------------------------------------------------------
# Sub-projects
# ----------------------------------------------------------------------------
add_subdirectory(wzlib)

if(CLIENT_SIM_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(CLIENT_SIM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ----------------------------------------------------------------------------
# Summary
# ----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "=== client_sim configuration ===")
message(STATUS "  Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  OpenSSL         : ${OPENSSL_VERSION} (${OPENSSL_INCLUDE_DIR})")
message(STATUS "  zlib            : ${ZLIB_VERSION_STRING}")
message(STATUS "  Qt6             : ${Qt6_VERSION} (found: ${Qt6_FOUND})")
message(STATUS "  Build tests     : ${CLIENT_SIM_BUILD_TESTS}")
message(STATUS "  Build tools     : ${CLIENT_SIM_BUILD_TOOLS}")
message(STATUS "  ASan / UBSan    : ${CLIENT_SIM_USE_ASAN} / ${CLIENT_SIM_USE_UBSAN}")
message(STATUS "================================")
message(STATUS "")
