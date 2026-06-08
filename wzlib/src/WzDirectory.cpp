#include "wzlib/WzDirectory.h"

#include "wzlib/Log.h"
#include "wzlib/Platform.h"
#include "wzlib/WzBinaryReader.h"
#include "wzlib/WzCrypto.h"
#include "wzlib/WzImage.h"

namespace wzlib {

WzDirectory* WzDirectory::find(const std::string& name) const {
    for (auto& c : children_) {
        if (c && c->name_ == name) return c.get();
    }
    return nullptr;
}

WzDirectory* WzDirectory::get(const std::string& path) const {
    if (path.empty()) return nullptr;
    WzDirectory* cur = const_cast<WzDirectory*>(this);
    size_t i = 0;
    while (i < path.size() && cur) {
        size_t j = path.find('/', i);
        if (j == i) { ++i; continue; }
        std::string seg = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
        i = (j == std::string::npos) ? path.size() : j + 1;
        if (seg.empty() || seg == ".") continue;
        if (seg == "..") {
            // No parent pointer; the caller uses getFromRoot on the file
            // for ".." support. Here we return null.
            return nullptr;
        }
        cur = cur->find(seg);
    }
    return cur;
}

Result<std::unique_ptr<WzDirectory>>
WzDirectoryReader::readBlock(WzBinaryReader* reader,
                             int64_t baseOffset,
                             WzCryptoContext* crypto,
                             int32_t blockSize) {
    if (!reader) return Result<std::unique_ptr<WzDirectory>>::error(ErrorCode::InvalidArgument);

    auto root = std::make_unique<WzDirectory>("");
    root->setOffset(baseOffset);

    const int64_t end = (blockSize > 0)
                          ? (baseOffset + blockSize)
                          : reader->length();

    while (reader->position() < end) {
        const int64_t entryPos = reader->position();
        const uint8_t nameLen = reader->readU8();
        // End-of-block marker: a name length of 0 means no more entries.
        if (nameLen == 0) {
            break;
        }
        // Encrypted name: nameLen bytes follow.
        std::string name(reinterpret_cast<const char*>(&nameLen), 0);
        // Re-read; the actual content of the name follows the nameLen byte.
        std::vector<uint8_t> nameBuf(nameLen);
        if (reader->read(nameBuf.data(), nameLen) != ErrorCode::Ok) {
            log::error("WzDirectory: short read on name");
            return Result<std::unique_ptr<WzDirectory>>::error(ErrorCode::IOError);
        }
        if (crypto) {
            crypto->decryptString(nameBuf.data(), nameBuf.size());
        }
        name.assign(nameBuf.begin(), nameBuf.end());

        // Strip any trailing NULs.
        while (!name.empty() && name.back() == '\0') name.pop_back();

        // 4 bytes: data size (size of WZImage content, or child count for
        // directory entries).
        const int32_t dataSize = reader->readI32();
        // 4 bytes: checksum (for WZImage content; 0 for directory).
        const int32_t cs32     = reader->readI32();
        // 8 bytes: 4-byte child count + 4-byte offset of child block.
        // (Older formats packed these as one 8-byte field; we read them
        // explicitly to be future-proof.)
        // For directory nodes, "data" is interpreted as the child block
        // count and the offset of that block, which is what we read next
        // as 8 bytes.
        const int32_t childCount = reader->readI32();
        const int32_t childOffset = reader->readI32();

        auto child = std::make_unique<WzDirectory>(name);
        child->setHash(static_cast<int32_t>(nameLen));  // best-effort
        child->setSize(dataSize);
        child->setCs32(cs32);
        child->setOffset(static_cast<int64_t>(childOffset));

        if (cs32 == 0 && childCount > 0) {
            // Directory entry: its "data" is the child block.
            if (reader->seek(static_cast<int64_t>(childOffset)) != ErrorCode::Ok) {
                return Result<std::unique_ptr<WzDirectory>>::error(ErrorCode::IOError);
            }
            auto subBlock = readBlock(reader, childOffset, crypto, childCount);
            if (subBlock.isError()) {
                return Result<std::unique_ptr<WzDirectory>>::error(subBlock.error());
            }
            // Move the parsed children from the synthetic sub-block into
            // this directory node.
            child->children() = std::move(subBlock.value()->children());
        }
        // If cs32 != 0, this is a leaf pointing at a WZImage — caller
        // will instantiate one. The data lives at childOffset, size bytes.

        root->addChild(std::move(child));

        // Resume scanning from where this entry ended (the readBlock
        // recursion above may have moved the position, so re-seek
        // back to right after this entry's metadata).
        if (reader->seek(entryPos + 1 + nameLen + 4 + 4 + 4 + 4) != ErrorCode::Ok) {
            return Result<std::unique_ptr<WzDirectory>>::error(ErrorCode::IOError);
        }
    }

    return Result<std::unique_ptr<WzDirectory>>(std::move(root));
}

}  // namespace wzlib
