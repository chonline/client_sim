#pragma once

/**
 * @file WzImage.h
 * @brief A lazy-loaded .img data block.
 *
 * A WzImage corresponds to a single .img entry in the WZ file. Its contents
 * are stored compressed (zlib) and possibly encrypted. The WzImage class
 * holds the offset and size in the parent WzFile, plus a copy of the
 * compressed bytes once they've been read in. The actual property tree is
 * constructed when tryExtract() is called.
 *
 * The root node is empty until tryExtract() succeeds; all children live as
 * WzNode* under rootNode().
 */

#include <cstdint>
#include <memory>
#include <vector>

#include "wzlib/ErrorCode.h"
#include "wzlib/WzNode.h"

namespace wzlib {

class WzBinaryReader;
class WzCryptoContext;

/**
 * @brief A WZ image block (.img).
 */
class WzImage {
public:
    WzImage() = default;

    WzImage(int64_t offset, int32_t size, int32_t cs32)
        : offset_(offset), size_(size), cs32_(cs32) {}

    // --- Identification -----------------------------------------------------
    int64_t offset() const { return offset_; }
    int32_t size()  const { return size_; }
    int32_t cs32()  const { return cs32_; }
    bool    extracted() const { return extracted_; }

    // --- Extraction ---------------------------------------------------------
    /**
     * @brief Decompress and parse the image contents into the property tree.
     *
     * Idempotent: returns Ok on an already-extracted image, no-op.
     * Returns CorruptedImage on data integrity failure (bad checksum or
     * structural inconsistency).
     */
    VoidResult tryExtract(WzBinaryReader* reader, WzCryptoContext* crypto);

    /** @brief Access the root property node of this image. */
    WzNode* rootNode() { return &root_; }
    const WzNode* rootNode() const { return &root_; }

    /**
     * @brief Mark all children of root_ as belonging to this image.
     * Called internally after tryExtract.
     */
    void bindOwningImage();

    /**
     * @brief Set the raw (decompressed but not yet parsed) bytes.
     * Used by readers to stage data before tryExtract.
     */
    void setRaw(std::vector<uint8_t> raw) { raw_ = std::move(raw); }

    /**
     * @brief Read-only access to the raw decompressed bytes.
     */
    const std::vector<uint8_t>& raw() const { return raw_; }

private:
    int64_t offset_ = 0;
    int32_t size_   = 0;
    int32_t cs32_   = 0;
    bool extracted_ = false;
    std::vector<uint8_t> raw_;     // Decompressed but unparsed.
    WzNode root_;                  // Root of the property tree (empty name).

    /**
     * @brief Internal helper: parse the property list starting at `pos`.
     */
    void parseProperty(WzNode* parent,
                       const uint8_t* data, size_t len, size_t& pos);
};

}  // namespace wzlib
