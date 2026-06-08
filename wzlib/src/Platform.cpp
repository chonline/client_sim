#include "wzlib/Platform.h"

#include <algorithm>
#include <chrono>
#include <vector>

namespace wzlib::platform {

std::string normalizePath(const std::string& path) {
    std::string out;
    out.reserve(path.size());

    std::vector<std::string> stack;
    for (size_t i = 0; i < path.size(); ) {
        // Find next '/' or end.
        size_t j = path.find('/', i);
        std::string seg = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
        i = (j == std::string::npos) ? path.size() : j + 1;

        if (seg.empty() || seg == ".") {
            continue;
        }
        if (seg == "..") {
            if (!stack.empty()) {
                stack.pop_back();
            }
            // .. above root is silently dropped.
            continue;
        }
        stack.push_back(std::move(seg));
    }

    for (size_t i = 0; i < stack.size(); ++i) {
        if (i > 0) out += '/';
        out += stack[i];
    }
    return out;
}

std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;

    bool aEnds = a.back() == '/' || a.back() == '\\';
    bool bStarts = b.front() == '/' || b.front() == '\\';
    if (aEnds && bStarts) {
        return a + b.substr(1);
    }
    if (!aEnds && !bStarts) {
        return a + '/' + b;
    }
    return a + b;
}

std::string dirName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return "";
    return path.substr(0, pos);
}

std::string baseName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < path.size()) {
        size_t j = path.find('/', i);
        std::string seg = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
        if (!seg.empty()) {
            out.push_back(std::move(seg));
        }
        i = (j == std::string::npos) ? path.size() : j + 1;
    }
    return out;
}

int64_t currentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace wzlib::platform
