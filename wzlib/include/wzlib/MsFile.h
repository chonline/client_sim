#pragma once

/**
 * @file MsFile.h
 * @brief The new (post-2013) MS resource file format.
 *
 * MS files are functionally similar to WZ files but use a slightly
 * different on-disk structure: the first 4 bytes are a "PACK" tag, the
 * next 4 are the file count, and each entry is a 0x100-byte directory
 * block. The actual format varies per region; this is a stub.
 *
 * For M1 we declare the API and leave the implementation for M2.
 */

#include <memory>
#include <string>
#include <vector>

#include "wzlib/ErrorCode.h"

namespace wzlib {

/**
 * @brief Stub for the new MS resource file format.
 *
 * Will be fully implemented in a later milestone. The class exists so
 * the public API surface is stable from day one.
 */
class MsFile {
public:
    static Result<std::unique_ptr<MsFile>> open(const std::string& path);
    bool isOpen() const { return open_; }
    const std::string& path() const { return path_; }

private:
    std::string path_;
    bool open_ = false;
    std::vector<uint8_t> raw_;
};

}  // namespace wzlib
