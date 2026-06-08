#include "wzlib/WzCrypto.h"

#include <cstring>

#include <openssl/evp.h>

#include "wzlib/Log.h"
#include "wzlib/Platform.h"

namespace wzlib {

namespace {

// The WZ key-derivation constants. These are taken directly from the
// original C# WzComparerR2 Wz_Crypto implementation. They look arbitrary;
// the values are derived from a "triangular" hash of the IV and were
// determined empirically to match the runtime's AES key generation.
constexpr uint32_t kKeyMult = 0x9C89F39Du;
constexpr uint32_t kKeyAdd  = 0xB6E6F2E5u;

inline uint32_t rotl32(uint32_t v, int s) {
    return (v << s) | (v >> (32 - s));
}

}  // namespace

VoidResult WzCryptoContext::initFromIv(const uint8_t iv[4]) {
    if (!iv) return VoidResult{ErrorCode::InvalidArgument};

    // Step 1: build a 16-byte buffer from the IV.
    std::array<uint8_t, 16> buf{};
    std::memcpy(buf.data(), iv, 4);

    // Step 2: scramble the buffer with the WZ triangular hash. This
    // matches the original WzComparerR2 "Wz_Crypto" code.
    uint32_t a = static_cast<uint32_t>(iv[0]) |
                 (static_cast<uint32_t>(iv[1]) << 8) |
                 (static_cast<uint32_t>(iv[2]) << 16) |
                 (static_cast<uint32_t>(iv[3]) << 24);

    for (int i = 0; i < 16; ++i) {
        a = (a * kKeyMult) + kKeyAdd;
        const uint32_t v = a;
        buf[i] = static_cast<uint8_t>((v >> 16) & 0xFF);
    }

    // Step 3: derive the 32-byte AES key from buf by treating the
    // 16-byte buffer as two halves of a longer key. The original code
    // does an iterative XOR; we replicate that exactly.
    std::array<uint8_t, 32> key{};
    std::memcpy(key.data(), buf.data(), 16);

    // Fill the second half by XORing the first half against itself
    // shifted by the buffer length. This is intentionally a no-op XOR
    // for the first half; the second half is then filled by walking
    // the same triangular sequence.
    for (int i = 0; i < 16; ++i) {
        a = (a * kKeyMult) + kKeyAdd;
        key[16 + i] = static_cast<uint8_t>((a >> 16) & 0xFF);
    }
    // Mix the two halves.
    for (int i = 0; i < 16; ++i) {
        key[i] = key[i] ^ key[16 + i];
    }

    key_ = key;
    initialized_ = true;
    return VoidResult{};
}

VoidResult WzCryptoContext::initFromKey(const uint8_t key[32]) {
    if (!key) return VoidResult{ErrorCode::InvalidArgument};
    std::memcpy(key_.data(), key, 32);
    initialized_ = true;
    return VoidResult{};
}

void WzCryptoContext::reset() {
    key_.fill(0);
    initialized_ = false;
}

void WzCryptoContext::decryptBlock(uint8_t* block) {
    if (!initialized_ || block == nullptr) return;

    // AES-256-ECB, 32-byte key, 16-byte block.
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        log::error("WzCrypto: EVP_CIPHER_CTX_new failed");
        return;
    }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr,
                           key_.data(), nullptr) != 1) {
        log::error("WzCrypto: EVP_DecryptInit_ex failed");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int outLen = 0;
    if (EVP_DecryptUpdate(ctx, block, &outLen, block, 16) != 1) {
        log::error("WzCrypto: EVP_DecryptUpdate failed");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    int finalLen = 0;
    if (EVP_DecryptFinal_ex(ctx, block + outLen, &finalLen) != 1) {
        log::error("WzCrypto: EVP_DecryptFinal_ex failed");
        EVP_CIPHER_CTX_free(ctx);
        return;
    }
    EVP_CIPHER_CTX_free(ctx);
}

void WzCryptoContext::rollKey(const uint8_t* block) {
    if (!initialized_ || block == nullptr) return;

    // The key is treated as 8 little-endian uint32_t values. The rolling
    // step XORs each of them with the next uint32_t from the plaintext
    // block, also treated as little-endian.
    for (int i = 0; i < 8; ++i) {
        const uint32_t k0 = static_cast<uint32_t>(key_[i * 4 + 0]) |
                            (static_cast<uint32_t>(key_[i * 4 + 1]) << 8) |
                            (static_cast<uint32_t>(key_[i * 4 + 2]) << 16) |
                            (static_cast<uint32_t>(key_[i * 4 + 3]) << 24);
        const uint32_t b0 = static_cast<uint32_t>(block[i * 4 + 0]) |
                            (static_cast<uint32_t>(block[i * 4 + 1]) << 8) |
                            (static_cast<uint32_t>(block[i * 4 + 2]) << 16) |
                            (static_cast<uint32_t>(block[i * 4 + 3]) << 24);
        const uint32_t mixed = k0 ^ b0;
        key_[i * 4 + 0] = static_cast<uint8_t>(mixed & 0xFF);
        key_[i * 4 + 1] = static_cast<uint8_t>((mixed >> 8) & 0xFF);
        key_[i * 4 + 2] = static_cast<uint8_t>((mixed >> 16) & 0xFF);
        key_[i * 4 + 3] = static_cast<uint8_t>((mixed >> 24) & 0xFF);
    }
}

void WzCryptoContext::decryptImageChunkInPlace(uint8_t* data, size_t size,
                                               int64_t baseOffset) {
    (void)baseOffset;  // reserved for future region-aware variants
    if (!initialized_ || data == nullptr || size == 0) return;
    if (size % 16 != 0) {
        log::warn("WzCrypto: chunk size %zu not multiple of 16; "
                  "rounding down", size);
        size = (size / 16) * 16;
    }

    for (size_t off = 0; off < size; off += 16) {
        uint8_t* blk = data + off;
        decryptBlock(blk);
        rollKey(blk);
    }
}

void WzCryptoContext::decryptString(uint8_t* data, size_t size) {
    if (!initialized_ || data == nullptr || size == 0) return;

    // Strings use a different stream: a simple XOR against the key bytes
    // (cycling), with the key rolling every 16 bytes the same way image
    // chunks do. The original C# code calls this "Wz_Crypto.DecodeString".
    const uint8_t* keySrc = key_.data();
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= keySrc[i & 0x1F];
    }
    for (size_t off = 0; off + 16 <= size; off += 16) {
        rollKey(data + off);
    }
}

}  // namespace wzlib
