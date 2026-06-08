#include "wzlib/WzImage.h"

#include <zlib.h>

#include <cstring>
#include <functional>
#include <memory>

#include "wzlib/Log.h"
#include "wzlib/WzBinaryReader.h"
#include "wzlib/WzCrypto.h"
#include "wzlib/WzDouble.h"
#include "wzlib/WzInt.h"
#include "wzlib/WzObject.h"
#include "wzlib/WzPng.h"
#include "wzlib/WzSound.h"
#include "wzlib/WzString.h"
#include "wzlib/WzUol.h"
#include "wzlib/WzVector.h"

namespace wzlib {

// Forward declarations for helpers used by parseProperty.
static bool looksLikeMp3(const uint8_t* data, size_t len);
static std::unique_ptr<WzObject> parsePngValue(const std::vector<uint8_t>& pngData);

VoidResult WzImage::tryExtract(WzBinaryReader* reader, WzCryptoContext* crypto) {
    if (extracted_) return VoidResult{};
    if (!reader) return VoidResult{ErrorCode::InvalidArgument};

    // 1. Read compressed bytes from file.
    std::vector<uint8_t> compressed(static_cast<size_t>(size_));
    if (reader->seek(offset_) != ErrorCode::Ok) {
        return VoidResult{ErrorCode::IOError};
    }
    if (reader->read(compressed.data(), compressed.size()) != ErrorCode::Ok) {
        return VoidResult{ErrorCode::IOError};
    }

    // 2. Decrypt (image chunks).
    if (crypto) {
        crypto->decryptImageChunkInPlace(compressed.data(), compressed.size(), offset_);
    }

    // 3. zlib decompress.
    std::vector<uint8_t> decompressed;
    {
        z_stream strm{};
        if (inflateInit(&strm) != Z_OK) {
            return VoidResult{ErrorCode::ZlibDecompressFailed};
        }
        strm.next_in  = compressed.data();
        strm.avail_in = static_cast<uInt>(compressed.size());

        std::vector<uint8_t> out;
        out.reserve(static_cast<size_t>(size_) * 4);  // guess
        uint8_t buf[8192];
        int ret = Z_OK;
        do {
            strm.next_out  = buf;
            strm.avail_out = sizeof(buf);
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                inflateEnd(&strm);
                return VoidResult{ErrorCode::ZlibDecompressFailed};
            }
            out.insert(out.end(), buf, buf + (sizeof(buf) - strm.avail_out));
        } while (ret != Z_STREAM_END);
        inflateEnd(&strm);
        decompressed = std::move(out);
    }

    // 4. (Optional) verify checksum.
    if (cs32_ != 0) {
        int32_t actual = WzBinaryReader::computeChecksum(decompressed.data(),
                                                         decompressed.size());
        if (actual != cs32_) {
            log::debug("WzImage checksum mismatch: expected 0x%08X got 0x%08X",
                       cs32_, actual);
            // Don't fail outright — some KMS WZ files have wrong checksums
            // and the original C# code still reads them.
        }
    }

    raw_ = std::move(decompressed);
    extracted_ = true;

    // 5. Parse the property tree.
    size_t pos = 0;
    try {
        parseProperty(&root_, raw_.data(), raw_.size(), pos);
    } catch (...) {
        extracted_ = false;
        raw_.clear();
        return VoidResult{ErrorCode::CorruptedImage};
    }
    bindOwningImage();
    return VoidResult{};
}

void WzImage::bindOwningImage() {
    // Mark every descendant as belonging to this image.
    std::function<void(WzNode*)> visit = [&](WzNode* n) {
        n->owningImage_ = this;
        for (auto& [_, child] : n->children()) {
            visit(child.get());
        }
    };
    visit(&root_);
}

void WzImage::parseProperty(WzNode* parent,
                            const uint8_t* data, size_t len, size_t& pos) {
    if (pos >= len) return;
    const int32_t entryCount = WzBinaryReader::readCompressedInt(data, len, pos);
    if (entryCount < 0 || pos + static_cast<size_t>(entryCount) * 2 > 1024 * 1024) {
        throw std::runtime_error("implausible entry count");
    }
    for (int32_t i = 0; i < entryCount; ++i) {
        // Read name (possibly shared via a string pool, but we ignore that
        // optimization for now — just read a fresh string each time).
        std::string name;
        if (!WzBinaryReader::readStringAt(data, len, pos, name)) {
            throw std::runtime_error("failed to read property name");
        }
        if (pos >= len) throw std::runtime_error("unexpected EOF before type tag");

        const uint8_t typeTag = data[pos++];
        auto child = std::make_unique<WzNode>(name, parent);

        switch (typeTag) {
            case 0x00:  // Null
                break;
            case 0x02: {  // Short (2 bytes, signed)
                if (pos + 2 > len) throw std::runtime_error("EOF in short");
                const int16_t v = static_cast<int16_t>(
                    static_cast<uint16_t>(data[pos]) |
                    (static_cast<uint16_t>(data[pos + 1]) << 8));
                pos += 2;
                child->setValue(std::make_unique<WzInt>(static_cast<int32_t>(v)));
                break;
            }
            case 0x03: {  // Int
                if (pos + 4 > len) throw std::runtime_error("EOF in int");
                const int32_t v = static_cast<int32_t>(
                    static_cast<uint32_t>(data[pos]) |
                    (static_cast<uint32_t>(data[pos + 1]) << 8) |
                    (static_cast<uint32_t>(data[pos + 2]) << 16) |
                    (static_cast<uint32_t>(data[pos + 3]) << 24));
                pos += 4;
                child->setValue(std::make_unique<WzInt>(v));
                break;
            }
            case 0x04: {  // Float
                if (pos + 4 > len) throw std::runtime_error("EOF in float");
                float v;
                std::memcpy(&v, data + pos, 4);
                pos += 4;
                child->setValue(std::make_unique<WzDouble>(static_cast<double>(v)));
                break;
            }
            case 0x05: {  // Double
                if (pos + 8 > len) throw std::runtime_error("EOF in double");
                double v;
                std::memcpy(&v, data + pos, 8);
                pos += 8;
                child->setValue(std::make_unique<WzDouble>(v));
                break;
            }
            case 0x08: {  // String
                std::string s;
                if (!WzBinaryReader::readStringAt(data, len, pos, s)) {
                    throw std::runtime_error("EOF in string");
                }
                child->setValue(std::make_unique<WzString>(std::move(s)));
                break;
            }
            case 0x09:  // Sub-property: recurse.
            {
                parseProperty(child.get(), data, len, pos);
                break;
            }
            case 0x0B: {  // UOL
                std::string s;
                if (!WzBinaryReader::readStringAt(data, len, pos, s)) {
                    throw std::runtime_error("EOF in uol");
                }
                child->setValue(std::make_unique<WzUol>(std::move(s)));
                break;
            }
            case 0x10: {  // Vector2D
                if (pos + 8 > len) throw std::runtime_error("EOF in vector");
                const int32_t x = static_cast<int32_t>(
                    static_cast<uint32_t>(data[pos]) |
                    (static_cast<uint32_t>(data[pos + 1]) << 8) |
                    (static_cast<uint32_t>(data[pos + 2]) << 16) |
                    (static_cast<uint32_t>(data[pos + 3]) << 24));
                const int32_t y = static_cast<int32_t>(
                    static_cast<uint32_t>(data[pos + 4]) |
                    (static_cast<uint32_t>(data[pos + 5]) << 8) |
                    (static_cast<uint32_t>(data[pos + 6]) << 16) |
                    (static_cast<uint32_t>(data[pos + 7]) << 24));
                pos += 8;
                child->setValue(std::make_unique<WzVector>(x, y));
                break;
            }
            case 0x19: {  // WzPng (canvas)
                if (pos + 4 > len) throw std::runtime_error("EOF in png len");
                const int32_t dataLen = static_cast<int32_t>(
                    static_cast<uint32_t>(data[pos]) |
                    (static_cast<uint32_t>(data[pos + 1]) << 8) |
                    (static_cast<uint32_t>(data[pos + 2]) << 16) |
                    (static_cast<uint32_t>(data[pos + 3]) << 24));
                pos += 4;
                if (pos + dataLen > len) throw std::runtime_error("EOF in png data");
                std::vector<uint8_t> pngData(data + pos, data + pos + dataLen);
                pos += dataLen;
                child->setValue(parsePngValue(pngData));
                break;
            }
            case 0x1B: {  // Sound
                if (pos + 8 > len) throw std::runtime_error("EOF in sound");
                const int32_t dataLen = static_cast<int32_t>(
                    static_cast<uint32_t>(data[pos]) |
                    (static_cast<uint32_t>(data[pos + 1]) << 8) |
                    (static_cast<uint32_t>(data[pos + 2]) << 16) |
                    (static_cast<uint32_t>(data[pos + 3]) << 24));
                const int32_t durMs = static_cast<int32_t>(
                    static_cast<uint32_t>(data[pos + 4]) |
                    (static_cast<uint32_t>(data[pos + 5]) << 8) |
                    (static_cast<uint32_t>(data[pos + 6]) << 16) |
                    (static_cast<uint32_t>(data[pos + 7]) << 24));
                pos += 8;
                if (pos + dataLen > len) throw std::runtime_error("EOF in sound data");
                WzSound::Type st = (dataLen >= 4 &&
                                    looksLikeMp3(data + pos, static_cast<size_t>(dataLen)))
                                       ? WzSound::Type::MP3
                                       : WzSound::Type::PCM;
                std::vector<uint8_t> soundData(data + pos, data + pos + dataLen);
                pos += dataLen;
                child->setValue(std::make_unique<WzSound>(st, durMs, std::move(soundData)));
                break;
            }
            default:
                throw std::runtime_error("unknown property type tag");
        }

        parent->addChild(std::move(child));
    }
}

// Helper: detect MP3 by ID3v2 header or frame sync word.
static bool looksLikeMp3(const uint8_t* data, size_t len) {
    if (data == nullptr || len < 4) return false;
    // ID3v2: "ID3" + 2 version bytes.
    if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') return true;
    // MP3 frame sync: 11 bits set, 2 bits version, 2 bits layer, 1 bit
    // error protection. The first byte is 0xFF (all 8 bits set) and the
    // top 3 bits of the second byte are also set (so byte[0] == 0xFF
    // and (byte[1] & 0xE0) == 0xE0).
    if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) return true;
    return false;
}

static std::unique_ptr<WzObject> parsePngValue(const std::vector<uint8_t>& pngData) {
    // Png block format: 1 byte format, 2 bytes width (LE), 2 bytes height (LE),
    // then 4 bytes that may be a strip offset (KMS) or a u32 padded data,
    // followed by the actual pixel data.
    if (pngData.size() < 9) return nullptr;

    const int32_t fmtInt = static_cast<int32_t>(pngData[0]);
    const int32_t width  = static_cast<int32_t>(pngData[1] | (pngData[2] << 8));
    const int32_t height = static_cast<int32_t>(pngData[3] | (pngData[4] << 8));

    // The remaining bytes are the format-specific pixel data. KMS files
    // sometimes interleave a 4-byte length prefix after the header; we
    // don't try to skip it for the M1 pass.
    constexpr size_t dataStart = 5;

    std::vector<uint8_t> raw(pngData.begin() + dataStart, pngData.end());
    return std::make_unique<WzPng>(static_cast<WzPng::Format>(fmtInt),
                                    width, height, std::move(raw));
}

}  // namespace wzlib
