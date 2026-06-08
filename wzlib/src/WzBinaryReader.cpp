#include "wzlib/WzBinaryReader.h"

#include <cstring>

#include "wzlib/Log.h"
#include "wzlib/Platform.h"

namespace wzlib {

// --- File I/O ---------------------------------------------------------------

VoidResult WzBinaryReader::open(const std::string& path) {
    if (stream_.is_open()) close();
    path_ = path;
    stream_.open(path, std::ios::binary | std::ios::ate);
    if (!stream_.is_open()) {
        log::error("WzBinaryReader: cannot open %s", path.c_str());
        return VoidResult{ErrorCode::FileNotFound};
    }
    length_ = static_cast<int64_t>(stream_.tellg());
    stream_.seekg(0, std::ios::beg);
    return VoidResult{};
}

void WzBinaryReader::close() {
    if (stream_.is_open()) stream_.close();
    path_.clear();
    length_ = 0;
}

VoidResult WzBinaryReader::seek(int64_t offset) {
    if (!stream_.is_open()) return VoidResult{ErrorCode::NotInitialized};
    stream_.seekg(offset, std::ios::beg);
    if (stream_.fail()) {
        return VoidResult{ErrorCode::IOError};
    }
    return VoidResult{};
}

VoidResult WzBinaryReader::skip(int64_t count) {
    if (!stream_.is_open()) return VoidResult{ErrorCode::NotInitialized};
    stream_.seekg(count, std::ios::cur);
    if (stream_.fail()) {
        return VoidResult{ErrorCode::IOError};
    }
    return VoidResult{};
}

VoidResult WzBinaryReader::read(uint8_t* dst, size_t count) {
    if (!stream_.is_open()) return VoidResult{ErrorCode::NotInitialized};
    if (count == 0) return VoidResult{};
    stream_.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(count));
    if (static_cast<size_t>(stream_.gcount()) != count) {
        return VoidResult{ErrorCode::IOError};
    }
    return VoidResult{};
}

uint8_t WzBinaryReader::readU8() {
    uint8_t b = 0;
    if (read(&b, 1) != ErrorCode::Ok) return 0;
    return b;
}

uint16_t WzBinaryReader::readU16() {
    uint8_t buf[2] = {0, 0};
    if (read(buf, 2) != ErrorCode::Ok) return 0;
    if (platform::isLittleEndian()) {
        return static_cast<uint16_t>(buf[0] | (buf[1] << 8));
    }
    return static_cast<uint16_t>((buf[0] << 8) | buf[1]);
}

int32_t WzBinaryReader::readI32() {
    uint8_t buf[4] = {0, 0, 0, 0};
    if (read(buf, 4) != ErrorCode::Ok) return 0;
    uint32_t v = static_cast<uint32_t>(buf[0]) |
                 (static_cast<uint32_t>(buf[1]) << 8) |
                 (static_cast<uint32_t>(buf[2]) << 16) |
                 (static_cast<uint32_t>(buf[3]) << 24);
    return static_cast<int32_t>(v);
}

std::vector<uint8_t> WzBinaryReader::readBytes(size_t count) {
    std::vector<uint8_t> out(count);
    if (count == 0) return out;
    if (read(out.data(), count) != ErrorCode::Ok) {
        out.clear();
    }
    return out;
}

// --- WZ compressed int ------------------------------------------------------

int32_t WzBinaryReader::readCompressedInt(const uint8_t* data, size_t len,
                                          size_t& pos) {
    if (data == nullptr || pos >= len) return 0;
    const uint8_t b0 = data[pos];
    if (b0 < 0x80) {
        pos += 1;
        return static_cast<int32_t>(b0);
    }
    if (b0 < 0xC0) {
        if (pos + 2 > len) { pos = len; return 0; }
        const int32_t v = ((b0 & 0x3F) << 8) | data[pos + 1];
        pos += 2;
        return v;
    }
    if (b0 < 0xE0) {
        if (pos + 4 > len) { pos = len; return 0; }
        const int32_t v = ((b0 & 0x1F) << 24) |
                          (data[pos + 1] << 16) |
                          (data[pos + 2] << 8)  |
                          data[pos + 3];
        pos += 4;
        return v;
    }
    // 0xFF sentinel for the full 32-bit negative case.
    if (pos + 5 > len) { pos = len; return 0; }
    const int32_t v = static_cast<int32_t>(
        static_cast<uint32_t>(data[pos + 1]) |
        (static_cast<uint32_t>(data[pos + 2]) << 8) |
        (static_cast<uint32_t>(data[pos + 3]) << 16) |
        (static_cast<uint32_t>(data[pos + 4]) << 24));
    pos += 5;
    return v;
}

// --- WZ string --------------------------------------------------------------

bool WzBinaryReader::readStringAt(const uint8_t* data, size_t len, size_t& pos,
                                  std::string& out, int32_t linkerIndex) {
    if (data == nullptr || pos >= len) return false;
    int32_t strLen = readCompressedInt(data, len, pos);
    if (strLen < 0) {
        // Negative => shared string (linker) index.
        const int32_t idx = -strLen;
        (void)idx;
        (void)linkerIndex;
        // Resolution is performed by the caller (WzImage) which has the
        // StringLinker. We can't know here whether it succeeded; the
        // signature is intentionally narrow.
        out.clear();
        return true;
    }
    if (pos + static_cast<size_t>(strLen) > len) return false;
    out.assign(reinterpret_cast<const char*>(data + pos),
               static_cast<size_t>(strLen));
    pos += static_cast<size_t>(strLen);
    return true;
}

// --- Checksum ---------------------------------------------------------------

const std::vector<int32_t>& WzBinaryReader::checksumTable() {
    static std::vector<int32_t> table;
    static bool initialized = false;
    if (!initialized) {
        table.resize(256);
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = static_cast<int32_t>(c);
        }
        initialized = true;
    }
    return table;
}

int32_t WzBinaryReader::computeChecksum(const uint8_t* data, size_t len) {
    const auto& table = checksumTable();
    int32_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum = (sum >> 8) ^ table[(sum ^ data[i]) & 0xFF];
    }
    return sum;
}

}  // namespace wzlib
