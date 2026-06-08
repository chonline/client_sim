#include "wztest/wztest.h"

#include "wzlib/WzCrypto.h"

TEST(wzcrypto, init_from_iv) {
    wzlib::WzCryptoContext c;
    uint8_t iv[4] = {0x4B, 0x31, 0x00, 0x00};
    EXPECT_OK(c.initFromIv(iv));
    EXPECT_TRUE(c.isInitialized());
}

TEST(wzcrypto, init_from_iv_zero) {
    wzlib::WzCryptoContext c;
    uint8_t iv[4] = {0, 0, 0, 0};
    EXPECT_OK(c.initFromIv(iv));
}

TEST(wzcrypto, init_from_key) {
    wzlib::WzCryptoContext c;
    uint8_t key[32] = {0};
    for (int i = 0; i < 32; ++i) key[i] = static_cast<uint8_t>(i);
    EXPECT_OK(c.initFromKey(key));
    EXPECT_EQ(static_cast<int>(c.key()[0]), 0);
    EXPECT_EQ(static_cast<int>(c.key()[31]), 31);
}

TEST(wzcrypto, decrypt_string_in_place) {
    wzlib::WzCryptoContext c;
    uint8_t iv[4] = {0x4B, 0x31, 0x00, 0x00};
    EXPECT_OK(c.initFromIv(iv));
    // Build a plaintext, encrypt manually by XOR with the same key, then
    // verify decryptString reverses it.
    std::vector<uint8_t> data = {'h', 'e', 'l', 'l', 'o', ' ',
                                  'w', 'o', 'r', 'l', 'd'};
    const auto& key = c.key();
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i & 0x1F];
    }
    c.decryptString(data.data(), data.size());
    // After decrypt, the bytes should be the original "hello world".
    const std::string expected = "hello world";
    EXPECT_TRUE(std::memcmp(data.data(), expected.data(), expected.size()) == 0);
}

TEST(wzcrypto, reset) {
    wzlib::WzCryptoContext c;
    uint8_t iv[4] = {0x4B, 0x31, 0x00, 0x00};
    EXPECT_OK(c.initFromIv(iv));
    EXPECT_TRUE(c.isInitialized());
    c.reset();
    EXPECT_FALSE(c.isInitialized());
}

RUN_TESTS(wzcrypto)
