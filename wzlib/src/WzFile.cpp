#include "wzlib/WzFile.h"

#include <algorithm>

#include "wzlib/Log.h"
#include "wzlib/StringLinker.h"
#include "wzlib/WzImage.h"

namespace wzlib {

WzFile::~WzFile() = default;

Result<std::unique_ptr<WzFile>> WzFile::open(const std::string& path) {
    auto file = std::unique_ptr<WzFile>(new WzFile());
    VoidResult openRes = file->reader_.open(path);
    if (openRes.isError()) {
        return Result<std::unique_ptr<WzFile>>::error(openRes.error());
    }
    if (file->reader_.length() < 60) {
        return Result<std::unique_ptr<WzFile>>::error(ErrorCode::CorruptedHeader);
    }

    // 1. Parse the header.
    if (file->reader_.seek(0) != ErrorCode::Ok) {
        return Result<std::unique_ptr<WzFile>>::error(ErrorCode::IOError);
    }
    auto hdrRes = WzHeaderParser::read(&file->reader_);
    if (hdrRes.isError()) {
        return Result<std::unique_ptr<WzFile>>::error(hdrRes.error());
    }
    file->header_ = std::move(hdrRes.value());

    // 2. Initialize the crypto context from the IV.
    auto cryptoRes = file->crypto_.initFromIv(file->header_.iv.data());
    if (cryptoRes.isError()) {
        return Result<std::unique_ptr<WzFile>>::error(cryptoRes.error());
    }

    // 3. The directory block is at the end of the file: last 4 bytes
    //    are a little-endian int32 that is the offset of the start
    //    of the directory tree.
    if (file->reader_.seek(file->reader_.length() - 4) != ErrorCode::Ok) {
        return Result<std::unique_ptr<WzFile>>::error(ErrorCode::IOError);
    }
    const int32_t dirOffset = file->reader_.readI32();

    if (file->reader_.seek(dirOffset) != ErrorCode::Ok) {
        return Result<std::unique_ptr<WzFile>>::error(ErrorCode::IOError);
    }
    // The first thing in the directory block is a 4-byte size prefix.
    const int32_t dirSize = file->reader_.readI32();
    if (dirSize <= 0) {
        return Result<std::unique_ptr<WzFile>>::error(ErrorCode::CorruptedDirectory);
    }
    auto dirRes = WzDirectoryReader::readBlock(&file->reader_, dirOffset + 4,
                                                &file->crypto_, dirSize);
    if (dirRes.isError()) {
        return Result<std::unique_ptr<WzFile>>::error(dirRes.error());
    }
    file->root_ = std::move(dirRes.value());

    // 4. Index the leaf images.
    file->indexImages(file->root_.get(), "");

    log::info("WzFile: opened %s (sig=%.4s version=%d images=%zu)",
              path.c_str(),
              file->header_.signature.c_str(),
              file->header_.version,
              file->images_.size());
    return Result<std::unique_ptr<WzFile>>(std::move(file));
}

void WzFile::indexImages(WzDirectory* dir, const std::string& prefix) {
    if (!dir) return;
    for (auto& child : dir->children()) {
        if (!child) continue;
        std::string path = prefix.empty() ? child->name() :
                                            prefix + "/" + child->name();

        // A leaf is any entry whose cs32 != 0 OR whose size > 0 AND
        // no children. We use a simple convention: directories have
        // children, images do not.
        if (child->children().empty() && child->size() > 0) {
            auto img = std::make_unique<WzImage>(child->offset(),
                                                  child->size(),
                                                  child->cs32());
            images_[path] = std::move(img);
        } else {
            indexImages(child.get(), path);
        }
    }
}

WzDirectory* WzFile::findDirectory(const std::string& path) const {
    if (!root_) return nullptr;
    return root_->get(path);
}

WzImage* WzFile::findImage(const std::string& path) {
    auto it = images_.find(path);
    if (it == images_.end()) return nullptr;
    return it->second.get();
}

std::vector<std::string> WzFile::allImagePaths() const {
    std::vector<std::string> out;
    out.reserve(images_.size());
    for (const auto& [k, _] : images_) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

VoidResult WzFile::loadPropertyImage() {
    // The "Property" image is the master string pool; in our model it
    // is exposed as a StringLinker. For M1 we instantiate a linker but
    // do not require it to be present.
    linker_ = std::make_unique<StringLinker>();
    return VoidResult{};
}

void WzFile::setStringLinker(std::unique_ptr<StringLinker> l) {
    linker_ = std::move(l);
}

}  // namespace wzlib
