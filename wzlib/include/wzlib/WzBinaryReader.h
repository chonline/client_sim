#pragma once

/**
 * @file WzBinaryReader.h
 * @brief Buffered file reader + binary parsing helpers for WZ files.
 *
 * WzBinaryReader wraps an std::ifstream (lazily opened) and provides the
 * binary primitives used throughout the WZ parser:
 *   - seek / read / readU8 / readU16 / readU32 / readI32
 *   - readCompressedInt — variable-length integer (1, 2, 4, or 9 bytes)
 *   - readStringAt       — WZ "shared string" or inline string
 *   - computeChecksum    — the WZ CRC-32 used for image integrity
 *
 * The reader is intentionally stateful: it owns the file handle. Multiple
 * readers can be created against the same path; that is fine because WZ
 * files are read-only.
 */

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "wzlib/ErrorCode.h"

namespace wzlib {

/**
 * @brief Lazy, forward-only file reader for WZ files.
 */
class WzBinaryReader {
public:
    WzBinaryReader() = default;

    /**
     * @brief Open the file at @p path. Idempotent on success: re-opening
     *        an already-open reader will seek back to position 0.
     */
    VoidResult open(const std::string& path);

    /** @brief True if the underlying file is open. */
    bool isOpen() const { return stream_.is_open(); }

    /** @brief Close the file (if open). */
    void close();

    /** @brief Current absolute file offset. */
    int64_t position() const {
        return stream_.is_open()
            ? static_cast<int64_t>(const_cast<std::ifstream&>(stream_).tellg())
            : 0;
    }

    /** @brief Total file size in bytes. */
    int64_t length() const { return length_; }

    // --- File I/O -----------------------------------------------------------
    /**
     * @brief Seek to an absolute offset.
     * @return Ok on success, IOError on failure.
     */
    VoidResult seek(int64_t offset);

    /**
     * @brief Skip forward by @p count bytes (relative to current position).
     */
    VoidResult skip(int64_t count);

    /**
     * @brief Read @p count bytes into @p dst.
     * @return Ok on success, IOError on short read / stream error.
     */
    VoidResult read(uint8_t* dst, size_t count);

    /**
     * @brief Read a single byte. Returns 0 on error.
     */
    uint8_t readU8();

    /** @brief Read a little-endian uint16. Returns 0 on error. */
    uint16_t readU16();

    /** @brief Read a little-endian int32. Returns 0 on error. */
    int32_t readI32();

    /**
     * @brief Read exactly @p count bytes and return them as a vector.
     */
    std::vector<uint8_t> readBytes(size_t count);

    // --- WZ-specific helpers ----------------------------------------------
    /**
     * @brief Read a WZ "compressed int" starting at @p pos.
     *
     * Encoding (1, 2, 4, or 9 bytes):
     *   first byte < 0x80                : value = first byte
     *   first byte < 0xC0, second < 0x80 : value = ((first & 0x3F) << 8) | second
     *   first byte < 0xE0                : value = ((first & 0x1F) << 24) |
     *                                            (second << 16) |
     *                                            (third << 8)  | fourth
     *   else                             : value = read 4 bytes as int32
     *                                       (used for negative values)
     *
     * @p pos is advanced past the int. Returns 0 on error.
     */
    static int32_t readCompressedInt(const uint8_t* data, size_t len, size_t& pos);

    /**
     * @brief Read a WZ string starting at @p pos.
     *
     * Two cases:
     *   1. Inline: first byte is the string length; followed by UTF-8 bytes.
     *   2. Shared: first byte is 0; followed by a 4-byte index into the
     *      StringLinker pool.
     *
     * When @p linker is non-null, indices are resolved against it; the
     * actual string content is then copied into @p out. When the linker
     * is null, the index case returns an empty string.
     *
     * @return true on success, false on truncated / malformed input.
     */
    static bool readStringAt(const uint8_t* data, size_t len, size_t& pos,
                             std::string& out, int32_t linkerIndex = -1);

    /**
     * @brief Compute the WZ CRC-32 over @p data.
     *
     * The WZ "checksum" is a custom 32-bit polynomial: it is the standard
     * CRC-32 (poly 0xEDB88320) but with the initial value and final XOR
     * adjusted so that the result matches what the original C# library
     * produces. We use a precomputed table for speed.
     */
    static int32_t computeChecksum(const uint8_t* data, size_t len);

    /**
     * @brief Return the CRC-32 lookup table (computed on first call).
     */
    static const std::vector<int32_t>& checksumTable();

    /** @brief Path this reader was opened from (or "" if not opened). */
    const std::string& path() const { return path_; }

private:
    std::ifstream stream_;
    std::string path_;
    int64_t length_ = 0;
};

}  // namespace wzlib
