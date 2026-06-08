#pragma once

/**
 * @file WzDirectory.h
 * @brief One entry in a WZ file's directory tree.
 *
 * A WzDirectory is a record on disk: 2-byte name hash, 2-byte name
 * length, N bytes of name (encrypted string), 4-byte data size,
 * 4-byte checksum, 4-byte child count. The data the directory points
 * to is a sequence of child directory entries, with a final tail
 * block holding the "image" data of the leaf nodes.
 *
 * In our model, parsing the file builds an in-memory tree of WzDirectory
 * objects; the leaves are WzImage instances whose contents are extracted
 * lazily.
 */

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "wzlib/ErrorCode.h"

namespace wzlib {

class WzBinaryReader;
class WzCryptoContext;

/**
 * @brief In-memory representation of a directory entry.
 *
 * Once parsed, a directory entry is a small POD with the on-disk
 * metadata plus its children. The WzImage data block referenced by
 * leaf directories is held in a WzImage child; the bytes themselves
 * stay on disk until tryExtract() is called.
 */
class WzDirectory {
public:
    WzDirectory() = default;
    WzDirectory(std::string name) : name_(std::move(name)) {}

    // --- Identification -----------------------------------------------------
    const std::string& name() const { return name_; }
    void setName(std::string n) { name_ = std::move(n); }

    /** @brief On-disk hash of the entry name. */
    int32_t hash() const { return hash_; }
    void setHash(int32_t h) { hash_ = h; }

    /** @brief WZImage payload size, or 0 for non-leaf nodes. */
    int32_t size() const { return size_; }
    void setSize(int32_t s) { size_ = s; }

    /** @brief WZImage payload checksum, or 0 for non-leaf nodes. */
    int32_t cs32() const { return cs32_; }
    void setCs32(int32_t c) { cs32_ = c; }

    /** @brief Offset in the parent WzFile where the entry's data starts. */
    int64_t offset() const { return offset_; }
    void setOffset(int64_t o) { offset_ = o; }

    // --- Tree structure -----------------------------------------------------
    const std::vector<std::unique_ptr<WzDirectory>>& children() const {
        return children_;
    }
    std::vector<std::unique_ptr<WzDirectory>>& children() {
        return children_;
    }
    void addChild(std::unique_ptr<WzDirectory> child) {
        if (child) children_.push_back(std::move(child));
    }
    int32_t childCount() const { return static_cast<int32_t>(children_.size()); }

    /**
     * @brief Find a direct child by name. Returns nullptr if not found.
     */
    WzDirectory* find(const std::string& name) const;

    /**
     * @brief Find a child by path (forward slashes, with optional leading
     *        slash and ".." segments). Returns nullptr if any segment is
     *        missing.
     */
    WzDirectory* get(const std::string& path) const;

private:
    std::string name_;
    int32_t hash_ = 0;
    int32_t size_ = 0;
    int32_t cs32_ = 0;
    int64_t offset_ = 0;
    std::vector<std::unique_ptr<WzDirectory>> children_;
};

/**
 * @brief Reader that parses a directory block off the wire.
 */
class WzDirectoryReader {
public:
    /**
     * @brief Read a directory block from @p reader.
     *
     * Reads a sequence of directory entries, with the last one being a
     * special "end of block" entry whose name starts with a non-printable
     * character.
     *
     * @param reader        The binary reader to consume bytes from.
     * @param baseOffset    The on-disk offset where this block starts.
     * @param crypto        Optional crypto context. If set, the names of
     *                      child entries are decrypted as they're read.
     * @param blockSize     The number of bytes the block is expected to
     *                      occupy. The reader stops once it's been
     *                      consumed. -1 means "read until end-of-block".
     * @return              A new WzDirectory on success (it is the
     *                      synthetic "block root" and contains the
     *                      children as its children_).
     */
    static Result<std::unique_ptr<WzDirectory>>
    readBlock(WzBinaryReader* reader,
              int64_t baseOffset,
              WzCryptoContext* crypto,
              int32_t blockSize);
};

}  // namespace wzlib
