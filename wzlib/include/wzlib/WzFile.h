#pragma once

/**
 * @file WzFile.h
 * @brief A single .wz file (one of the WZ bundle files on disk).
 *
 * A WzFile owns:
 *   - A WzBinaryReader (for low-level I/O).
 *   - A WzCryptoContext (keyed on the file's IV).
 *   - The root WzDirectory tree (parsed up front from the directory block
 *     at the end of the file).
 *   - A flat map of WzImage* by name (the WZ images referenced by the
 *     directory leaves, ready to be extracted on demand).
 *   - A WzImage for the top-level "Property" entry, which holds the
 *     global directory of names.
 *
 * Lifecycle:
 *   1. open(path) → parse header, init crypto, parse directory tree.
 *   2. User calls get(path) to navigate.
 *   3. User calls tryExtract() on a WzImage to decompress and parse its
 *      contents.
 *
 * The .wz files in a MapleStory client are interdependent: a WzFile
 * knows about its peers (WzStructure). The image *resolution* is
 * therefore a two-step process: first within the file, then across
 * the bundle. The WzFile API does not know about cross-file resolution
 * — that's WzStructure's job.
 */

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wzlib/ErrorCode.h"
#include "wzlib/WzBinaryReader.h"
#include "wzlib/WzCrypto.h"
#include "wzlib/WzDirectory.h"
#include "wzlib/WzHeader.h"

namespace wzlib {

class WzImage;

/**
 * @brief A single .wz file in a MapleStory install.
 */
class WzFile {
public:
    WzFile() = default;
    ~WzFile();

    /**
     * @brief Open the file at @p path and parse its directory tree.
     *
     * On success, the file is fully open and ready for navigation.
     * On failure, the object is in an error state and isOpen() is false.
     */
    static Result<std::unique_ptr<WzFile>> open(const std::string& path);

    /** @brief True after a successful open(). */
    bool isOpen() const { return reader_.isOpen(); }

    /** @brief Path to the underlying file on disk. */
    const std::string& path() const { return reader_.path(); }

    // --- Header / crypto --------------------------------------------------
    const WzHeader& header() const { return header_; }
    WzCryptoContext& crypto() { return crypto_; }
    const WzCryptoContext& crypto() const { return crypto_; }

    // --- Directory tree ---------------------------------------------------
    WzDirectory* root() { return root_.get(); }
    const WzDirectory* root() const { return root_.get(); }

    /**
     * @brief Find a directory entry by path (e.g. "Map/Back/0").
     */
    WzDirectory* findDirectory(const std::string& path) const;

    /**
     * @brief Find or create the WzImage at @p path.
     *
     * If the path doesn't correspond to a leaf in this file, returns
     * nullptr. Otherwise the returned image is not yet extracted;
     * call tryExtract() on it to read its bytes.
     */
    WzImage* findImage(const std::string& path);

    /**
     * @brief All image paths known to this file (sorted).
     */
    std::vector<std::string> allImagePaths() const;

    // --- Low-level access (for advanced users) ---------------------------
    WzBinaryReader& reader() { return reader_; }
    const WzBinaryReader& reader() const { return reader_; }

    /**
     * @brief Read the global "Property" image at the end of the file.
     * This holds the master directory of string-pool entries.
     */
    VoidResult loadPropertyImage();

    /**
     * @brief Get the global string linker (from Property.img).
     * May be null if the property image hasn't been loaded.
     */
    class StringLinker* stringLinker() const { return linker_.get(); }
    void setStringLinker(std::unique_ptr<class StringLinker> l);

private:
    WzBinaryReader reader_;
    WzHeader header_{};
    WzCryptoContext crypto_;
    std::unique_ptr<WzDirectory> root_;
    std::unordered_map<std::string, std::unique_ptr<WzImage>> images_;
    std::unique_ptr<class StringLinker> linker_;

    /**
     * @brief Walk the directory tree, instantiating WzImage leaves.
     *
     * Recursively populates images_ from the parsed WzDirectory nodes.
     */
    void indexImages(WzDirectory* dir, const std::string& prefix);
};

}  // namespace wzlib
