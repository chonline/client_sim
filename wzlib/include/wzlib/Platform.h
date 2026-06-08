#pragma once

/**
 * @file Platform.h
 * @brief Platform abstraction layer.
 *
 * wzlib deliberately does not depend on Qt. This file provides the small
 * surface of platform-specific functionality the library needs: path
 * normalization, default font discovery, time queries, and endianness.
 *
 * The UI layer (which uses Qt) wraps these for the platforms where Qt is
 * available.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace wzlib::platform {

/**
 * @brief Normalize a path: forward slashes only, no trailing separator,
 *        collapse ".." / "." components. UTF-8 in/out.
 */
std::string normalizePath(const std::string& path);

/**
 * @brief Join two path components with a single '/'.
 *
 *   joinPath("Map", "Back")       => "Map/Back"
 *   joinPath("Map/", "/Back")     => "Map/Back"
 */
std::string joinPath(const std::string& a, const std::string& b);

/**
 * @brief Get the directory portion of a path. "" if no '/'.
 */
std::string dirName(const std::string& path);

/**
 * @brief Get the file name portion of a path.
 */
std::string baseName(const std::string& path);

/**
 * @brief Split a path by '/'. Empty segments are dropped.
 */
std::vector<std::string> splitPath(const std::string& path);

/**
 * @brief Current time in milliseconds since an arbitrary epoch.
 *
 * Used for timing measurements (open time, decode time, etc.). Wall clock
 * based; resolution is OS-dependent but typically ~1ms.
 */
int64_t currentTimeMs();

/**
 * @brief Compile-time endianness check.
 */
constexpr bool isLittleEndian() {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return true;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return false;
#elif defined(_MSC_VER)
    return true;  // MSVC always targets little-endian.
#else
    // Best-effort fallback.
    const uint16_t v = 0x0001;
    return *reinterpret_cast<const uint8_t*>(&v) == 0x01;
#endif
}

/**
 * @brief Reverse bytes of a 16-bit value.
 */
constexpr uint16_t bswap16(uint16_t v) {
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}

/**
 * @brief Reverse bytes of a 32-bit value.
 */
constexpr uint32_t bswap32(uint32_t v) {
    return ((v >> 24) & 0x000000FFu) | ((v >> 8) & 0x0000FF00u) |
           ((v << 8) & 0x00FF0000u) | ((v << 24) & 0xFF000000u);
}

/**
 * @brief Reverse bytes of a 64-bit value.
 */
constexpr uint64_t bswap64(uint64_t v) {
    return ((v >> 56) & 0x00000000000000FFull) |
           ((v >> 40) & 0x000000000000FF00ull) |
           ((v >> 24) & 0x0000000000FF0000ull) |
           ((v >> 8)  & 0x00000000FF000000ull) |
           ((v << 8)  & 0x000000FF00000000ull) |
           ((v << 24) & 0x0000FF0000000000ull) |
           ((v << 40) & 0x00FF000000000000ull) |
           ((v << 56) & 0xFF00000000000000ull);
}

}  // namespace wzlib::platform
