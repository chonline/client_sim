#pragma once

/**
 * @file WzCrypto.h
 * @brief AES-256-ECB crypto context used by WZ files.
 *
 * WZ files are not just compressed; the bytes (or, in some KMS variants,
 * 16-byte chunks) are also XORed / AES-decrypted with a key derived from a
 * per-file initialization vector. The IV is the first 4 bytes of the
 * encrypted region of the file.
 *
 * WzCryptoContext owns the 32-byte AES key and the 16-byte "AES block" used
 * to feed the cipher. Each call to decryptImageChunkInPlace() walks the
 * 16-byte chunks of the image, decrypts them, and rolls the key forward
 * to the next chunk. The rolling step is what makes the cipher "context
 * dependent" — the same plaintext chunk encrypted at offset 0 vs offset
 * 16 will produce different ciphertext.
 *
 * The key generation algorithm is the one used by MapleStory WZ since
 * the KMS overhaul (~2013) and also covers the older "no chunk" variant.
 */

#include <array>
#include <cstdint>
#include <vector>

#include "wzlib/ErrorCode.h"

namespace wzlib {

/**
 * @brief AES-256-ECB crypto state, keyed on a per-file IV.
 */
class WzCryptoContext {
public:
    WzCryptoContext() = default;

    /**
     * @brief Initialize the context from a 4-byte IV.
     *
     * The IV is the first 4 bytes of the encrypted region of the WZ file
     * (or, for KMS files, the value 0x314B is prepended by the writer).
     * After this call, the context can decrypt image chunks.
     */
    VoidResult initFromIv(const uint8_t iv[4]);

    /**
     * @brief Initialize from a full 16-byte key (some variants pre-bake
     *        the key and pass it in directly).
     */
    VoidResult initFromKey(const uint8_t key[32]);

    /** @brief True once a key has been set. */
    bool isInitialized() const { return initialized_; }

    /** @brief Reset to uninitialized state. */
    void reset();

    /**
     * @brief Decrypt an image chunk in place.
     *
     * The WZ format stores images as one or more 16-byte-aligned AES blocks
     * of ciphertext, followed by the zlib stream. We walk block by block,
     * decrypting with the *current* key, then rolling the key forward by
     * XORing it with the next 16 bytes of plaintext. This is the same
     * pattern MapleStory uses.
     *
     * @param data   Buffer to decrypt in place. Must be a multiple of 16.
     * @param size   Buffer size in bytes.
     * @param baseOffset  File offset where this chunk lives (used for
     *                    key derivation; pass 0 if unknown).
     */
    void decryptImageChunkInPlace(uint8_t* data, size_t size, int64_t baseOffset);

    /**
     * @brief Decrypt a single WZ string (variable length).
     *
     * The format is: bytes are XORed against a rolling key byte. The
     * rolling key is advanced after every 16 bytes by mixing the
     * key with the next 16 bytes of *plaintext* — the standard KMS
     * "triangular" key stream.
     */
    void decryptString(uint8_t* data, size_t size);

    /**
     * @brief Get the current 32-byte AES key (debug / test only).
     */
    const std::array<uint8_t, 32>& key() const { return key_; }

private:
    /**
     * @brief Roll the AES key forward by XORing it with @p block.
     *
     * @p block is treated as a 16-byte stream of uint32_t values (LE);
     * the algorithm matches the original WzComparerR2 implementation.
     */
    void rollKey(const uint8_t* block);

    /**
     * @brief Decrypt one 16-byte block in place using the current key.
     *        Caller is responsible for key rolling.
     */
    void decryptBlock(uint8_t* block);

    std::array<uint8_t, 32> key_{};
    bool initialized_ = false;
};

}  // namespace wzlib
