#pragma once

/**
 * @file WzHeader.h
 * @brief The first 60-ish bytes of a WZ file: signature, version, copyright.
 *
 * A WZ file begins with a 4-byte signature (PKG1 or PKG2), followed by a
 * 4-byte little-endian version, a 4-byte copyright year, then 44 bytes of
 * "WzFile" string and a 16-byte extra section. The exact layout depends
 * on the version, but the first 14 bytes are constant.
 *
 * The header is also where the per-file IV lives for the KMS variant —
 * it's the first 4 bytes of the encrypted region (right after the header
 * bytes we've parsed). For old-format WZ the IV is at a fixed offset.
 */

#include <array>
#include <cstdint>
#include <string>

#include "wzlib/ErrorCode.h"

namespace wzlib {

/**
 * @brief Parsed WZ file header.
 */
struct WzHeader {
    /** @brief "PKG1" or "PKG2" — pre-2013 vs post-2013 layout. */
    std::string signature;

    /** @brief WZ format version (60-65 for KMS, 64-69 for old KMS). */
    int32_t version = 0;

    /** @brief Copyright year (e.g. 2012). */
    int32_t copyrightYear = 0;

    /**
     * @brief 16-byte "tag" that is part of the AES key derivation in
     *        some KMS variants. For PKG2 it's 16 zero bytes; for PKG1
     *        it can be the file's content root name.
     */
    std::array<uint8_t, 16> tag{};

    /**
     * @brief Per-file 4-byte IV for the AES context. Read from the file
     *        immediately after the header.
     */
    std::array<uint8_t, 4> iv{};

    /** @brief True if this is the modern PKG2 format. */
    bool isPkg2() const { return signature == "PKG2"; }

    /** @brief True if this is the legacy PKG1 format. */
    bool isPkg1() const { return signature == "PKG1"; }
};

/**
 * @brief Parser for WZ file headers.
 */
class WzHeaderParser {
public:
    /**
     * @brief Read the header from @p reader (which must be open).
     *
     * On success, the reader's position will be at the first byte of the
     * encrypted region (i.e. the IV).
     */
    static Result<WzHeader> read(class WzBinaryReader* reader);
};

}  // namespace wzlib
