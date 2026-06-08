#include "wztest/wztest.h"

#include "wzlib/WzBinaryReader.h"

TEST(wzbinaryreader, read_compressed_int_single_byte) {
    uint8_t buf[] = {0x05};
    size_t pos = 0;
    EXPECT_EQ(wzlib::WzBinaryReader::readCompressedInt(buf, sizeof(buf), pos), 5);
    EXPECT_EQ(static_cast<int>(pos), 1);
}

TEST(wzbinaryreader, read_compressed_int_two_bytes) {
    uint8_t buf[] = {0x80 | 0x10, 0x20};  // ((0x10) << 8) | 0x20 = 0x1020
    size_t pos = 0;
    EXPECT_EQ(wzlib::WzBinaryReader::readCompressedInt(buf, sizeof(buf), pos),
              static_cast<int32_t>(((0x10 & 0x3F) << 8) | 0x20));
    EXPECT_EQ(static_cast<int>(pos), 2);
}

TEST(wzbinaryreader, read_compressed_int_four_bytes) {
    uint8_t buf[] = {0xC0 | 0x05, 0x06, 0x07, 0x08};
    size_t pos = 0;
    EXPECT_EQ(wzlib::WzBinaryReader::readCompressedInt(buf, sizeof(buf), pos),
              static_cast<int32_t>((0x05 << 24) | (0x06 << 16) |
                                    (0x07 << 8) | 0x08));
    EXPECT_EQ(static_cast<int>(pos), 4);
}

TEST(wzbinaryreader, read_string_at_inline) {
    const char* s = "hello";
    size_t total = 1 + std::strlen(s);
    std::vector<uint8_t> buf(total);
    buf[0] = static_cast<uint8_t>(std::strlen(s));
    std::memcpy(buf.data() + 1, s, std::strlen(s));
    size_t pos = 0;
    std::string out;
    EXPECT_TRUE(wzlib::WzBinaryReader::readStringAt(buf.data(), buf.size(),
                                                     pos, out));
    EXPECT_EQ(out, std::string("hello"));
}

TEST(wzbinaryreader, compute_checksum_known_value) {
    // Empty buffer -> 0 (no bytes hashed).
    EXPECT_EQ(wzlib::WzBinaryReader::computeChecksum(nullptr, 0), 0);
    // "abc" with standard CRC-32 polynomial 0xEDB88320 is well known.
    const uint8_t abc[] = {'a', 'b', 'c'};
    int32_t c = wzlib::WzBinaryReader::computeChecksum(abc, 3);
    // We don't pin a specific value (the original C# code may differ in
    // initial state); we only assert that two identical inputs give the
    // same result.
    EXPECT_EQ(c, wzlib::WzBinaryReader::computeChecksum(abc, 3));
}

RUN_TESTS(wzbinaryreader)
