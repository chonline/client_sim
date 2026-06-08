# wzlib: WZ/MS resource file parser library
#
# This is the foundation of the project. It parses MapleStory WZ resource
# bundles and exposes them as in-memory tree structures. It depends only on
# OpenSSL (for AES decryption) and zlib (for decompression).
#
# It does NOT depend on Qt — that integration is layered on top in the UI
# module. Keeping wzlib Qt-free lets it be:
#   - compiled and tested on any platform
#   - used in headless / server / batch contexts
#   - benchmarked without GUI overhead

add_library(wzlib
    STATIC

    # Core types & utilities
    src/ErrorCode.cpp
    src/Log.cpp
    src/Platform.cpp

    # Object types
    src/WzObject.cpp
    src/WzInt.cpp
    src/WzDouble.cpp
    src/WzString.cpp
    src/WzVector.cpp
    src/WzUol.cpp
    src/WzSound.cpp
    src/WzPng.cpp
    src/PngDecoder.cpp

    # Tree structure
    src/WzNode.cpp
    src/WzImage.cpp
    src/WzHeader.cpp
    src/WzDirectory.cpp
    src/WzBinaryReader.cpp

    # File layer
    src/WzFile.cpp
    src/WzStructure.cpp
    src/MsFile.cpp

    # Crypto
    src/WzCrypto.cpp

    # String / utility
    src/StringLinker.cpp
    src/Calculator.cpp
    src/StringResult.cpp
)

target_include_directories(wzlib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(wzlib
    PUBLIC
        OpenSSL::Crypto
        ZLIB::ZLIB
)

target_compile_features(wzlib PUBLIC cxx_std_17)

# Strict warning level
if(MSVC)
    target_compile_options(wzlib PRIVATE /W4 /permissive-)
else()
    target_compile_options(wzlib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Use C++17 filesystem (we may need it for path normalization)
if(NOT MSVC)
    target_link_libraries(wzlib PUBLIC stdc++fs)
endif()

# Position-independent for embedding in shared libs
set_target_properties(wzlib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# ----------------------------------------------------------------------------
# Header self-containment check
# ----------------------------------------------------------------------------
# When enabled, compiles a tiny test that includes each public header to
# verify it has no hidden dependencies on other wzlib headers.
option(CLIENT_SIM_HEADER_CHECK "Verify wzlib headers are self-contained" OFF)
if(CLIENT_SIM_HEADER_CHECK AND CLIENT_SIM_BUILD_TESTS)
    add_executable(wzlib_header_check tests/wzlib_header_check.cpp)
    target_link_libraries(wzlib_header_check PRIVATE wzlib)
    add_test(NAME wzlib_header_check COMMAND wzlib_header_check)
endif()
