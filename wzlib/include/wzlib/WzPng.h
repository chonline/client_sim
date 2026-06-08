#pragma once

#include <cstdint>
#include <vector>

#include "wzlib/WzObject.h"

namespace wzlib {

/**
 * @brief Bitmap image data.
 *
 * The WZ format stores images as compressed (and possibly encrypted) bitmaps
 * in one of several pixel formats. WzPng owns the *raw* pixel data (already
 * decompressed and decrypted) and a format tag. Call decode() to get an
 * RGBA8 pixel buffer.
 *
 * For very common formats, decode() is fast (linear scan). For compressed
 * formats (DXT1/3/5, BC7) it does block-based decoding.
 *
 * Pixel buffer layout: 8 bits per channel, R, G, B, A in that order, with
 * rows stored top-to-bottom and no padding. The buffer is owned by the
 * WzPng and is invalidated by any non-const operation.
 */
class WzPng : public WzObject {
public:
    /** @brief Pixel format tags from the WZ file format spec. */
    enum class Format : int32_t {
        Unknown       = 0,
        // 16-bit formats
        ARGB4444      = 0x02,
        ARGB1555      = 0x06,
        RGB565        = 0x07,
        // 32-bit formats
        ARGB8888      = 0x04,
        // Compressed (DXT/BC family)
        DXT1          = 0x201,
        DXT3          = 0x205,
        DXT5          = 0x209,
        BC7           = 0x20D,
        // 16-bit high precision
        RGBA1010102   = 0x1002,
        R16           = 0x1011,
        A8            = 0x1012,
        RGBA32Float   = 0x1015,
    };

    WzPng(Format fmt, int32_t width, int32_t height, std::vector<uint8_t> raw)
        : format_(fmt), width_(width), height_(height), raw_(std::move(raw)) {}

    ObjectType type() const override { return ObjectType::Png; }
    const WzPng* asPng() const override { return this; }

    Format format() const { return format_; }
    int32_t width()  const { return width_; }
    int32_t height() const { return height_; }

    /** @brief Raw, format-specific pixel data (already decrypted/decompressed). */
    const std::vector<uint8_t>& raw() const { return raw_; }

    /**
     * @brief Decode to an RGBA8 buffer (top-to-bottom, 4 bytes per pixel).
     *
     * Returns an empty vector on failure (e.g. malformed data, missing
     * decoder for a format we don't support). The result is cached so
     * repeated calls are free.
     */
    const std::vector<uint8_t>& decodeRgba() const;

    /**
     * @brief Get the decoded size in bytes (0 if not yet decoded).
     */
    size_t rgbaSize() const { return decoded_.size(); }

    /** @brief Force re-decode (e.g. after format change in tests). */
    void invalidateCache() { decoded_.clear(); }

    /**
     * @brief Set the raw data (used when re-decoding WZ data).
     */
    void setRaw(std::vector<uint8_t> raw) { raw_ = std::move(raw); decoded_.clear(); }

    /** @brief Static helper: format name for diagnostics. */
    static const char* formatName(Format fmt);

private:
    Format format_;
    int32_t width_;
    int32_t height_;
    std::vector<uint8_t> raw_;
    mutable std::vector<uint8_t> decoded_;
};

}  // namespace wzlib
