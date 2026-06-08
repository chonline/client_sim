#pragma once

/**
 * @file PngDecoder.h
 * @brief Format-specific decoders that turn WzPng raw data into RGBA8.
 *
 * The decoders are stateless free functions. They take a pointer to the
 * source data (already decrypted and decompressed), its size, the image
 * dimensions, and a destination buffer. The destination buffer must be at
 * least width*height*4 bytes; it is *not* cleared on entry.
 *
 * Each decoder returns true on success and false on failure (e.g. wrong
 * size for a given image, or unsupported sub-format).
 *
 * Pixel buffer layout: top-to-bottom, left-to-right, 4 bytes per pixel
 * (R, G, B, A in that order, 8 bits per channel).
 */

#include <cstddef>
#include <cstdint>

namespace wzlib::png {

/** @brief Decode a 16-bit ARGB4444 image (4 bits per channel, alpha first). */
bool decodeARGB4444(const uint8_t* src, size_t srcLen,
                    int32_t width, int32_t height,
                    uint8_t* dst, size_t dstLen);

/** @brief Decode a 16-bit ARGB1555 image (1-bit alpha, 5 bits per RGB). */
bool decodeARGB1555(const uint8_t* src, size_t srcLen,
                    int32_t width, int32_t height,
                    uint8_t* dst, size_t dstLen);

/** @brief Decode a 16-bit RGB565 image (no alpha channel). */
bool decodeRGB565(const uint8_t* src, size_t srcLen,
                  int32_t width, int32_t height,
                  uint8_t* dst, size_t dstLen);

/** @brief Decode a 32-bit ARGB8888 image. */
bool decodeARGB8888(const uint8_t* src, size_t srcLen,
                    int32_t width, int32_t height,
                    uint8_t* dst, size_t dstLen);

/** @brief Decode a DXT1 (BC1) image: 4x4 blocks, 8 bytes each. */
bool decodeDXT1(const uint8_t* src, size_t srcLen,
                int32_t width, int32_t height,
                uint8_t* dst, size_t dstLen);

/** @brief Decode a DXT3 (BC2) image: 4x4 blocks, 16 bytes each, explicit alpha. */
bool decodeDXT3(const uint8_t* src, size_t srcLen,
                int32_t width, int32_t height,
                uint8_t* dst, size_t dstLen);

/** @brief Decode a DXT5 (BC3) image: 4x4 blocks, 16 bytes each, interpolated alpha. */
bool decodeDXT5(const uint8_t* src, size_t srcLen,
                int32_t width, int32_t height,
                uint8_t* dst, size_t dstLen);

/** @brief Decode an 8-bit alpha-only image (luminance used as alpha). */
bool decodeA8(const uint8_t* src, size_t srcLen,
              int32_t width, int32_t height,
              uint8_t* dst, size_t dstLen);

}  // namespace wzlib::png
