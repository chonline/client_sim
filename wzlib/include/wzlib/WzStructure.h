#pragma once

/**
 * @file WzStructure.h
 * @brief A bundle of .wz files (Base.wz, Map.wz, Character.wz, ...).
 *
 * A real MapleStory install has dozens of .wz files; they form a single
 * virtual namespace. WzStructure is the entry point for the application:
 * it knows the list of files, lazily opens them on first access, and
 * routes lookups across them.
 *
 * WzStructure also owns the "Property" string linker once it's loaded
 * from the first file (Property.wz). All other files can then resolve
 * their internal string-pool indices against this linker.
 */

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wzlib/ErrorCode.h"
#include "wzlib/WzFile.h"

namespace wzlib {

/**
 * @brief A bundle of .wz files.
 */
class WzStructure {
public:
    WzStructure() = default;

    /**
     * @brief Construct from a directory path; auto-discovers the .wz
     *        files in it.
     */
    static Result<std::unique_ptr<WzStructure>> open(const std::string& baseDir);

    /**
     * @brief Add a file to the bundle. The file is opened immediately;
     *        failures are propagated.
     */
    VoidResult addFile(const std::string& path);

    /**
     * @brief Get a WzFile by basename (e.g. "Base.wz").
     */
    WzFile* getFile(const std::string& name) const;

    /**
     * @brief Find the WzImage that owns @p path, searching across all
     *        files in the bundle.
     *
     * @return nullptr if no file has such an image.
     */
    WzImage* findImage(const std::string& path) const;

    /**
     * @brief Find the WzFile that owns @p path.
     */
    WzFile* findFileForImage(const std::string& path) const;

    /** @brief Number of files in the bundle. */
    size_t fileCount() const { return files_.size(); }

    /** @brief All file basenames (sorted). */
    std::vector<std::string> allFileNames() const;

    /** @brief Access the underlying files (for advanced uses). */
    const std::vector<std::unique_ptr<WzFile>>& files() const { return files_; }

    /**
     * @brief Resolve a string-pool index against the master linker.
     *
     * If the linker hasn't been loaded, this returns an empty string.
     */
    std::string resolveString(int32_t index) const;

private:
    std::vector<std::unique_ptr<WzFile>> files_;
    std::unordered_map<std::string, WzFile*> byName_;
};

}  // namespace wzlib
