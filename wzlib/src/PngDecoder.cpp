#include "wzlib/PngDecoder.h"

#include <cstring>

namespace wzlib::png {

namespace {

// Expand a 4-bit channel to 8 bits by replication (e.g. 0xA -> 0xAA).
inline uint8_t expand4to8(uint8_t v) { return static_cast<uint8_t>((v << 4) | v); }

// Expand a 5-bit channel to 8 bits.
inline uint8_t expand5to8(uint8_t v) { return static_cast<uint8_t>((v << 3) | (v >> 2)); }

// Expand a 6-bit channel to 8 bits.
inline uint8_t expand6to8(uint8_t v) { return static_cast<uint8_t>((v << 2) | (v >> 4)); }

// Read a little-endian uint16 from a byte buffer.
inline uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

// Read a little-endian uint32 from a byte buffer.
inline uint32_t readU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

// Convert RGB565 (packed) to an 8-bit RGB triplet.
inline void rgb565ToRgb(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = expand5to8(static_cast<uint8_t>((c >> 11) & 0x1F));
    g = expand6to8(static_cast<uint8_t>((c >> 5)  & 0x3F));
    b = expand5to8(static_cast<uint8_t>( c        & 0x1F));
}

}  // namespace

bool decodeARGB4444(const uint8_t* src, size_t srcLen,
                    int32_t width, int32_t height,
                    uint8_t* dst, size_t dstLen) {
    const size_t expected = static_cast<size_t>(width) * height * 2;
    if (srcLen < expected || dstLen < static_cast<size_t>(width) * height * 4) {
        return false;
    }
    for (int32_t i = 0; i < width * height; ++i) {
        const uint16_t v = readU16LE(src + i * 2);
        // The WZ format packs A:R:G:B with A in the high nibble.
        dst[i * 4 + 0] = expand4to8(static_cast<uint8_t>((v >> 8)  & 0x0F));
        dst[i * 4 + 1] = expand4to8(static_cast<uint8_t>((v >> 4)  & 0x0F));
        dst[i * 4 + 2] = expand4to8(static_cast<uint8_t>( v        & 0x0F));
        dst[i * 4 + 3] = expand4to8(static_cast<uint8_t>((v >> 12) & 0x0F));
    }
    return true;
}

bool decodeARGB1555(const uint8_t* src, size_t srcLen,
                    int32_t width, int32_t height,
                    uint8_t* dst, size_t dstLen) {
    const size_t expected = static_cast<size_t>(width) * height * 2;
    if (srcLen < expected || dstLen < static_cast<size_t>(width) * height * 4) {
        return false;
    }
    for (int32_t i = 0; i < width * height; ++i) {
        const uint16_t v = readU16LE(src + i * 2);
        dst[i * 4 + 0] = expand5to8(static_cast<uint8_t>((v >> 10) & 0x1F));
        dst[i * 4 + 1] = expand5to8(static_cast<uint8_t>((v >> 5)  & 0x1F));
        dst[i * 4 + 2] = expand5to8(static_cast<uint8_t>( v        & 0x1F));
        dst[i * 4 + 3] = ((v & 0x8000) ? 0xFF : 0x00);
    }
    return true;
}

bool decodeRGB565(const uint8_t* src, size_t srcLen,
                  int32_t width, int32_t height,
                  uint8_t* dst, size_t dstLen) {
    const size_t expected = static_cast<size_t>(width) * height * 2;
    if (srcLen < expected || dstLen < static_cast<size_t>(width) * height * 4) {
        return false;
    }
    for (int32_t i = 0; i < width * height; ++i) {
        const uint16_t v = readU16LE(src + i * 2);
        dst[i * 4 + 0] = expand5to8(static_cast<uint8_t>((v >> 11) & 0x1F));
        dst[i * 4 + 1] = expand6to8(static_cast<uint8_t>((v >> 5)  & 0x3F));
        dst[i * 4 + 2] = expand5to8(static_cast<uint8_t>( v        & 0x1F));
        dst[i * 4 + 3] = 0xFF;
    }
    return true;
}

bool decodeARGB8888(const uint8_t* src, size_t srcLen,
                    int32_t width, int32_t height,
                    uint8_t* dst, size_t dstLen) {
    const size_t expected = static_cast<size_t>(width) * height * 4;
    if (srcLen < expected || dstLen < expected) {
        return false;
    }
    std::memcpy(dst, src, expected);
    return true;
}

bool decodeA8(const uint8_t* src, size_t srcLen,
              int32_t width, int32_t height,
              uint8_t* dst, size_t dstLen) {
    const size_t expected = static_cast<size_t>(width) * height;
    if (srcLen < expected || dstLen < static_cast<size_t>(width) * height * 4) {
        return false;
    }
    for (int32_t i = 0; i < width * height; ++i) {
        const uint8_t a = src[i];
        dst[i * 4 + 0] = 0xFF;
        dst[i * 4 + 1] = 0xFF;
        dst[i * 4 + 2] = 0xFF;
        dst[i * 4 + 3] = a;
    }
    return true;
}

// ============================================================================
// DXT1 / DXT3 / DXT5 (BC1 / BC2 / BC3)
// ============================================================================

namespace {

// Write a 4x4 block of decoded pixels at block position (bx, by) in pixel
// coordinates. The destination image is clipped to (0, 0, width, height).
inline void writeBlock(uint8_t* dst, int32_t width, int32_t height,
                       int32_t bx, int32_t by,
                       const uint8_t block[16][4]) {
    for (int32_t y = 0; y < 4; ++y) {
        const int32_t py = by + y;
        if (py < 0 || py >= height) continue;
        for (int32_t x = 0; x < 4; ++x) {
            const int32_t px = bx + x;
            if (px < 0 || px >= width) continue;
            uint8_t* p = dst + (py * width + px) * 4;
            p[0] = block[y * 4 + x][0];
            p[1] = block[y * 4 + x][1];
            p[2] = block[y * 4 + x][2];
            p[3] = block[y * 4 + x][3];
        }
    }
}

// DXT1: 8 bytes per 4x4 block. First 4 bytes = two RGB565 colors,
// next 4 bytes = sixteen 2-bit indices (little-endian).
void decodeDxt1Block(const uint8_t* src, int32_t bx, int32_t by,
                     int32_t width, int32_t height, uint8_t* dst) {
    const uint16_t c0 = readU16LE(src);
    const uint16_t c1 = readU16LE(src + 2);
    const uint32_t idx = readU32LE(src + 4);

    uint8_t colors[4][4];
    rgb565ToRgb(c0, colors[0][0], colors[0][1], colors[0][2]);
    rgb565ToRgb(c1, colors[1][0], colors[1][1], colors[1][2]);

    if (c0 > c1) {
        // 4-color block.
        colors[2][0] = static_cast<uint8_t>((2 * colors[0][0] + colors[1][0]) / 3);
        colors[2][1] = static_cast<uint8_t>((2 * colors[0][1] + colors[1][1]) / 3);
        colors[2][2] = static_cast<uint8_t>((2 * colors[0][2] + colors[1][2]) / 3);
        colors[3][0] = static_cast<uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3);
        colors[3][1] = static_cast<uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3);
        colors[3][2] = static_cast<uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3);
        colors[0][3] = colors[1][3] = colors[2][3] = colors[3][3] = 0xFF;
    } else {
        // 3-color block with one transparent slot.
        colors[2][0] = static_cast<uint8_t>((colors[0][0] + colors[1][0]) / 2);
        colors[2][1] = static_cast<uint8_t>((colors[0][1] + colors[1][1]) / 2);
        colors[2][2] = static_cast<uint8_t>((colors[0][2] + colors[1][2]) / 2);
        colors[0][3] = colors[1][3] = colors[2][3] = 0xFF;
        colors[3][0] = colors[3][1] = colors[3][2] = colors[3][3] = 0x00;
    }

    uint8_t block[16][4];
    for (int32_t i = 0; i < 16; ++i) {
        const int32_t sel = (idx >> (i * 2)) & 0x3;
        block[i][0] = colors[sel][0];
        block[i][1] = colors[sel][1];
        block[i][2] = colors[sel][2];
        block[i][3] = colors[sel][3];
    }
    writeBlock(dst, width, height, bx, by, block);
}

// DXT3 alpha block: 8 bytes of explicit 4-bit alpha (16 nibbles).
void decodeDxt3Block(const uint8_t* src, int32_t bx, int32_t by,
                     int32_t width, int32_t height, uint8_t* dst) {
    // First 8 bytes = explicit alpha nibbles.
    uint8_t alphas[16];
    for (int32_t i = 0; i < 16; i += 2) {
        const uint8_t b = src[i / 2];
        alphas[i + 0] = expand4to8(static_cast<uint8_t>( b       & 0x0F));
        alphas[i + 1] = expand4to8(static_cast<uint8_t>((b >> 4) & 0x0F));
    }
    // Next 8 bytes = same as DXT1 RGB block.
    const uint8_t* rgb = src + 8;
    const uint16_t c0 = readU16LE(rgb);
    const uint16_t c1 = readU16LE(rgb + 2);
    const uint32_t idx = readU32LE(rgb + 4);

    uint8_t colors[4][3];
    rgb565ToRgb(c0, colors[0][0], colors[0][1], colors[0][2]);
    rgb565ToRgb(c1, colors[1][0], colors[1][1], colors[1][2]);

    // DXT3 always uses 4-color mode (alpha is explicit, no transparent slot).
    colors[2][0] = static_cast<uint8_t>((2 * colors[0][0] + colors[1][0]) / 3);
    colors[2][1] = static_cast<uint8_t>((2 * colors[0][1] + colors[1][1]) / 3);
    colors[2][2] = static_cast<uint8_t>((2 * colors[0][2] + colors[1][2]) / 3);
    colors[3][0] = static_cast<uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3);
    colors[3][1] = static_cast<uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3);
    colors[3][2] = static_cast<uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3);

    uint8_t block[16][4];
    for (int32_t i = 0; i < 16; ++i) {
        const int32_t sel = (idx >> (i * 2)) & 0x3;
        block[i][0] = colors[sel][0];
        block[i][1] = colors[sel][1];
        block[i][2] = colors[sel][2];
        block[i][3] = alphas[i];
    }
    writeBlock(dst, width, height, bx, by, block);
}

// DXT5 alpha block: 2 endpoint bytes + 16x3-bit indices.
void decodeDxt5Block(const uint8_t* src, int32_t bx, int32_t by,
                     int32_t width, int32_t height, uint8_t* dst) {
    const uint8_t a0 = src[0];
    const uint8_t a1 = src[1];
    // Next 6 bytes = 16 3-bit indices, little-endian bitstream.
    const uint64_t bits = static_cast<uint64_t>(src[2])        |
                          (static_cast<uint64_t>(src[3]) << 8) |
                          (static_cast<uint64_t>(src[4]) << 16) |
                          (static_cast<uint64_t>(src[5]) << 24) |
                          (static_cast<uint64_t>(src[6]) << 32) |
                          (static_cast<uint64_t>(src[7]) << 40);

    uint8_t alphas[8];
    alphas[0] = a0;
    alphas[1] = a1;
    if (a0 > a1) {
        for (int32_t i = 0; i < 6; ++i) {
            alphas[2 + i] = static_cast<uint8_t>(((6 - i) * a0 + (i + 1) * a1) / 7);
        }
    } else {
        for (int32_t i = 0; i < 4; ++i) {
            alphas[2 + i] = static_cast<uint8_t>(((4 - i) * a0 + (i + 1) * a1) / 5);
        }
        alphas[6] = 0;
        alphas[7] = 0xFF;
    }

    uint8_t pixelAlphas[16];
    for (int32_t i = 0; i < 16; ++i) {
        const int32_t sel = (bits >> (i * 3)) & 0x7;
        pixelAlphas[i] = alphas[sel];
    }

    // RGB block is the next 8 bytes (DXT1 style).
    const uint8_t* rgb = src + 8;
    const uint16_t c0 = readU16LE(rgb);
    const uint16_t c1 = readU16LE(rgb + 2);
    const uint32_t idx = readU32LE(rgb + 4);

    uint8_t colors[4][3];
    rgb565ToRgb(c0, colors[0][0], colors[0][1], colors[0][2]);
    rgb565ToRgb(c1, colors[1][0], colors[1][1], colors[1][2]);
    // DXT5 always uses 4-color mode.
    colors[2][0] = static_cast<uint8_t>((2 * colors[0][0] + colors[1][0]) / 3);
    colors[2][1] = static_cast<uint8_t>((2 * colors[0][1] + colors[1][1]) / 3);
    colors[2][2] = static_cast<uint8_t>((2 * colors[0][2] + colors[1][2]) / 3);
    colors[3][0] = static_cast<uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3);
    colors[3][1] = static_cast<uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3);
    colors[3][2] = static_cast<uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3);

    uint8_t block[16][4];
    for (int32_t i = 0; i < 16; ++i) {
        const int32_t sel = (idx >> (i * 2)) & 0x3;
        block[i][0] = colors[sel][0];
        block[i][1] = colors[sel][1];
        block[i][2] = colors[sel][2];
        block[i][3] = pixelAlphas[i];
    }
    writeBlock(dst, width, height, bx, by, block);
}

}  // namespace

bool decodeDXT1(const uint8_t* src, size_t srcLen,
                int32_t width, int32_t height,
                uint8_t* dst, size_t dstLen) {
    if (width <= 0 || height <= 0) return false;
    const int32_t blocksW = (width + 3) / 4;
    const int32_t blocksH = (height + 3) / 4;
    const size_t expected = static_cast<size_t>(blocksW) * blocksH * 8;
    if (srcLen < expected) return false;
    if (dstLen < static_cast<size_t>(width) * height * 4) return false;

    for (int32_t by = 0; by < blocksH; ++by) {
        for (int32_t bx = 0; bx < blocksW; ++bx) {
            const uint8_t* block = src + (static_cast<size_t>(by) * blocksW + bx) * 8;
            decodeDxt1Block(block, bx * 4, by * 4, width, height, dst);
        }
    }
    return true;
}

bool decodeDXT3(const uint8_t* src, size_t srcLen,
                int32_t width, int32_t height,
                uint8_t* dst, size_t dstLen) {
    if (width <= 0 || height <= 0) return false;
    const int32_t blocksW = (width + 3) / 4;
    const int32_t blocksH = (height + 3) / 4;
    const size_t expected = static_cast<size_t>(blocksW) * blocksH * 16;
    if (srcLen < expected) return false;
    if (dstLen < static_cast<size_t>(width) * height * 4) return false;

    for (int32_t by = 0; by < blocksH; ++by) {
        for (int32_t bx = 0; bx < blocksW; ++bx) {
            const uint8_t* block = src + (static_cast<size_t>(by) * blocksW + bx) * 16;
            decodeDxt3Block(block, bx * 4, by * 4, width, height, dst);
        }
    }
    return true;
}

bool decodeDXT5(const uint8_t* src, size_t srcLen,
                int32_t width, int32_t height,
                uint8_t* dst, size_t dstLen) {
    if (width <= 0 || height <= 0) return false;
    const int32_t blocksW = (width + 3) / 4;
    const int32_t blocksH = (height + 3) / 4;
    const size_t expected = static_cast<size_t>(blocksW) * blocksH * 16;
    if (srcLen < expected) return false;
    if (dstLen < static_cast<size_t>(width) * height * 4) return false;

    for (int32_t by = 0; by < blocksH; ++by) {
        for (int32_t bx = 0; bx < blocksW; ++bx) {
            const uint8_t* block = src + (static_cast<size_t>(by) * blocksW + bx) * 16;
            decodeDxt5Block(block, bx * 4, by * 4, width, height, dst);
        }
    }
    return true;
}

}  // namespace wzlib::png
