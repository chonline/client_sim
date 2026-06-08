#include "wztest/wztest.h"

#include "wzlib/PngDecoder.h"
#include "wzlib/WzPng.h"

using namespace wzlib;

TEST(pngdecoder, argb8888_round_trip) {
    // Build a 2x2 ARGB8888 image: red, green, blue, white.
    std::vector<uint8_t> src = {
        0xFF, 0x00, 0x00, 0xFF,   // red
        0x00, 0xFF, 0x00, 0xFF,   // green
        0x00, 0x00, 0xFF, 0xFF,   // blue
        0xFF, 0xFF, 0xFF, 0xFF,   // white
    };
    std::vector<uint8_t> dst(2 * 2 * 4, 0);
    EXPECT_TRUE(png::decodeARGB8888(src.data(), src.size(), 2, 2,
                                     dst.data(), dst.size()));
    EXPECT_EQ(static_cast<int>(dst[0]), 0xFF);   // R of red
    EXPECT_EQ(static_cast<int>(dst[1]), 0x00);   // G of red
    EXPECT_EQ(static_cast<int>(dst[2]), 0x00);   // B of red
    EXPECT_EQ(static_cast<int>(dst[3]), 0xFF);   // A of red
    EXPECT_EQ(static_cast<int>(dst[4 + 1]), 0xFF); // G of green
    EXPECT_EQ(static_cast<int>(dst[8 + 2]), 0xFF); // B of blue
    EXPECT_EQ(static_cast<int>(dst[12 + 0]), 0xFF); // R of white
    EXPECT_EQ(static_cast<int>(dst[12 + 3]), 0xFF); // A of white
}

TEST(pngdecoder, rgb565_round_trip) {
    // 2x2 RGB565: pick a known value (pure red = 0xF800).
    std::vector<uint8_t> src = {
        0x00, 0xF8,  // red
        0x00, 0xF8,  // red
        0x00, 0xF8,  // red
        0x00, 0xF8,  // red
    };
    std::vector<uint8_t> dst(2 * 2 * 4, 0);
    EXPECT_TRUE(png::decodeRGB565(src.data(), src.size(), 2, 2,
                                   dst.data(), dst.size()));
    EXPECT_EQ(static_cast<int>(dst[0]), 0xFF);  // R
    EXPECT_EQ(static_cast<int>(dst[1]), 0x00);  // G
    EXPECT_EQ(static_cast<int>(dst[2]), 0x00);  // B
}

TEST(pngdecoder, argb1555_alpha_bit) {
    // 1x1 ARGB1555: 1.0 alpha + red = 0xFC00 (1_11111_00000_00000).
    std::vector<uint8_t> src = {0x00, 0xFC};
    std::vector<uint8_t> dst(1 * 1 * 4, 0);
    EXPECT_TRUE(png::decodeARGB1555(src.data(), src.size(), 1, 1,
                                     dst.data(), dst.size()));
    EXPECT_EQ(static_cast<int>(dst[3]), 0xFF);  // full alpha
    EXPECT_EQ(static_cast<int>(dst[0]), 0xFF);  // R
}

TEST(pngdecoder, a8_alpha) {
    std::vector<uint8_t> src = {0x80};
    std::vector<uint8_t> dst(1 * 1 * 4, 0);
    EXPECT_TRUE(png::decodeA8(src.data(), src.size(), 1, 1,
                               dst.data(), dst.size()));
    // R=G=B=0, A=0x80.
    EXPECT_EQ(static_cast<int>(dst[3]), 0x80);
}

TEST(wzpng, decode_rgba_caches) {
    std::vector<uint8_t> raw(2 * 2 * 4, 0);
    // 4 ARGB8888 pixels, all white.
    for (int i = 0; i < 16; ++i) raw[i] = 0xFF;

    WzPng png(WzPng::Format::ARGB8888, 2, 2, std::move(raw));
    const auto& decoded1 = png.decodeRgba();
    EXPECT_EQ(static_cast<int>(decoded1.size()), 16);
    const auto& decoded2 = png.decodeRgba();
    EXPECT_TRUE(&decoded1 == &decoded2);  // cached
    EXPECT_EQ(static_cast<int>(decoded1[3]), 0xFF);
}

RUN_TESTS(pngdecoder)
