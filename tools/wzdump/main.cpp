/**
 * @file wzdump/main.cpp
 * @brief Dump a WZ file's directory tree.
 *
 * Usage:
 *   wzdump <path-to-wz-file> [--max-depth N] [--list-images]
 *
 *   --max-depth N    Limit the directory traversal to N levels deep.
 *                    Default: unlimited.
 *   --list-images    Print only the WzImage paths, not directory nodes.
 *
 * Exit codes:
 *   0  Success
 *   1  Bad arguments
 *   2  File open failure
 *   3  Parse failure
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "wzlib/Log.h"
#include "wzlib/WzFile.h"
#include "wzlib/WzStructure.h"

namespace {

void printUsage(const char* argv0) {
    std::fprintf(stderr,
        "Usage: %s <path-to-wz-file> [--max-depth N] [--list-images]\n"
        "  --max-depth N    Limit the directory traversal to N levels deep.\n"
        "  --list-images    Print only the WzImage paths, not directory nodes.\n",
        argv0);
}

void walk(const wzlib::WzDirectory* dir, int depth, int maxDepth,
          bool listImages) {
    if (!dir) return;
    if (maxDepth >= 0 && depth > maxDepth) return;

    for (const auto& child : dir->children()) {
        if (!child) continue;
        if (child->children().empty() && child->size() > 0) {
            if (listImages) {
                std::printf("IMG  %s  (size=%d cs32=0x%08X offset=%lld)\n",
                            child->name().c_str(),
                            child->size(), child->cs32(),
                            static_cast<long long>(child->offset()));
            } else {
                std::printf("IMG  %-40s size=%d cs32=0x%08X\n",
                            child->name().c_str(),
                            child->size(), child->cs32());
            }
        } else {
            if (!listImages) {
                std::printf("DIR  %s/\n", child->name().c_str());
            }
            walk(child.get(), depth + 1, maxDepth, listImages);
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    wzlib::log::setLevel(wzlib::log::Level::Info);

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string path = argv[1];
    int maxDepth = -1;
    bool listImages = false;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--max-depth" && i + 1 < argc) {
            maxDepth = std::atoi(argv[++i]);
        } else if (arg == "--list-images") {
            listImages = true;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", arg.c_str());
            printUsage(argv[0]);
            return 1;
        }
    }

    // Try as a single WZ file first, then as a directory of WZ files.
    auto f = wzlib::WzFile::open(path);
    if (f.isOk()) {
        std::printf("Opened WZ file: %s\n", path.c_str());
        std::printf("  signature: %.4s\n", f.value()->header().signature.c_str());
        std::printf("  version:   %d\n", f.value()->header().version);
        std::printf("  images:    %zu\n", f.value()->allImagePaths().size());
        std::printf("  root:\n");
        walk(f.value()->root(), 0, maxDepth, listImages);
        return 0;
    }

    auto s = wzlib::WzStructure::open(path);
    if (s.isOk()) {
        std::printf("Opened WZ directory: %s\n", path.c_str());
        std::printf("  files: %zu\n", s.value()->fileCount());
        for (const auto& name : s.value()->allFileNames()) {
            std::printf("  - %s\n", name.c_str());
        }
        return 0;
    }

    std::fprintf(stderr, "Failed to open '%s' as a WZ file or directory.\n",
                 path.c_str());
    return 2;
}
