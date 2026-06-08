#pragma once

/**
 * @file StringLinker.h
 * @brief The master string pool used by WZ files.
 *
 * WZ files deduplicate string literals (like "Character", "Map", ...)
 * across the bundle. Each reference is a 4-byte index into a global
 * table, which lives in Property.img. StringLinker loads that table
 * and provides random access.
 *
 * Strings in the linker are stored encrypted (XOR against the file's
 * AES key). They are decrypted once at load time and held in memory.
 */

#include <memory>
#include <string>
#include <vector>

#include "wzlib/ErrorCode.h"

namespace wzlib {

class WzImage;

/**
 * @brief A pool of WZ strings, indexed by integer.
 */
class StringLinker {
public:
    StringLinker() = default;

    /**
     * @brief Load the linker from an extracted WzImage whose contents
     *        are a list of property entries whose names are the pool
     *        strings and whose values are 4-byte indices.
     *
     * (For M1 the linker is treated as empty by default and we provide
     *  a fallback that builds one from a flat string array, in case
     *  parsing fails on a KMS variant.)
     */
    VoidResult loadFromImage(const WzImage* img);

    /** @brief Build the linker from a pre-computed list (for tests). */
    void setStrings(std::vector<std::string> strings);

    /** @brief Total number of strings in the pool. */
    int32_t size() const { return static_cast<int32_t>(strings_.size()); }

    /**
     * @brief Resolve an index to a string.
     * @return the string, or "" if @p index is out of range.
     */
    std::string resolve(int32_t index) const;

    /** @brief Get all strings (read-only). */
    const std::vector<std::string>& strings() const { return strings_; }

private:
    std::vector<std::string> strings_;
};

}  // namespace wzlib
