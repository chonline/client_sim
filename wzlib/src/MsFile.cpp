#include "wzlib/MsFile.h"

#include <fstream>

#include "wzlib/Log.h"

namespace wzlib {

Result<std::unique_ptr<MsFile>> MsFile::open(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        log::error("MsFile: cannot open %s", path.c_str());
        return Result<std::unique_ptr<MsFile>>::error(ErrorCode::FileNotFound);
    }
    const int64_t size = static_cast<int64_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    auto m = std::make_unique<MsFile>();
    m->path_ = path;
    m->raw_.resize(static_cast<size_t>(size));
    if (size > 0) {
        f.read(reinterpret_cast<char*>(m->raw_.data()), size);
    }
    m->open_ = true;
    log::info("MsFile: opened %s (%lld bytes)", path.c_str(),
              static_cast<long long>(size));
    return Result<std::unique_ptr<MsFile>>(std::move(m));
}

}  // namespace wzlib
