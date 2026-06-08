#include "wzlib/WzStructure.h"

#include <algorithm>
#include <filesystem>
#include <set>

#include "wzlib/Log.h"
#include "wzlib/Platform.h"
#include "wzlib/StringLinker.h"

namespace wzlib {

Result<std::unique_ptr<WzStructure>> WzStructure::open(const std::string& baseDir) {
    namespace fs = std::filesystem;
    auto st = std::make_unique<WzStructure>();

    std::error_code ec;
    if (!fs::is_directory(baseDir, ec)) {
        log::error("WzStructure: not a directory: %s", baseDir.c_str());
        return Result<std::unique_ptr<WzStructure>>::error(ErrorCode::FileNotFound);
    }

    // Open files in a deterministic order: Property first (it carries the
    // string linker), then everything else alphabetically.
    std::vector<std::string> paths;
    for (auto& e : fs::directory_iterator(baseDir, ec)) {
        if (!e.is_regular_file()) continue;
        const auto& p = e.path();
        if (p.extension() == ".wz") {
            paths.push_back(p.string());
        }
    }
    std::sort(paths.begin(), paths.end());

    // Move "Property.wz" (if present) to the front.
    auto propIt = std::find_if(paths.begin(), paths.end(),
        [](const std::string& s) { return platform::baseName(s) == "Property.wz"; });
    if (propIt != paths.end() && propIt != paths.begin()) {
        std::rotate(paths.begin(), propIt, propIt + 1);
    }

    for (const auto& p : paths) {
        VoidResult r = st->addFile(p);
        if (r.isError()) {
            log::warn("WzStructure: skipping %s (%s)",
                      p.c_str(), toString(r.error()));
        }
    }

    if (st->files_.empty()) {
        return Result<std::unique_ptr<WzStructure>>::error(ErrorCode::FileNotFound);
    }
    return Result<std::unique_ptr<WzStructure>>(std::move(st));
}

VoidResult WzStructure::addFile(const std::string& path) {
    auto f = WzFile::open(path);
    if (f.isError()) return VoidResult{f.error()};

    WzFile* raw = f.value().get();
    const std::string name = platform::baseName(path);
    byName_[name] = raw;
    files_.push_back(std::move(f).value());
    return VoidResult{};
}

WzFile* WzStructure::getFile(const std::string& name) const {
    auto it = byName_.find(name);
    if (it == byName_.end()) return nullptr;
    return it->second;
}

WzImage* WzStructure::findImage(const std::string& path) const {
    for (auto& f : files_) {
        if (WzImage* img = f->findImage(path)) return img;
    }
    return nullptr;
}

WzFile* WzStructure::findFileForImage(const std::string& path) const {
    for (auto& f : files_) {
        if (f->findImage(path)) return f.get();
    }
    return nullptr;
}

std::vector<std::string> WzStructure::allFileNames() const {
    std::vector<std::string> out;
    out.reserve(byName_.size());
    for (const auto& [k, _] : byName_) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

std::string WzStructure::resolveString(int32_t index) const {
    for (const auto& f : files_) {
        if (auto* l = f->stringLinker()) {
            return l->resolve(index);
        }
    }
    return {};
}

}  // namespace wzlib
