#include "wzlib/WzHeader.h"

#include <cstring>

#include "wzlib/Log.h"
#include "wzlib/WzBinaryReader.h"

namespace wzlib {

Result<WzHeader> WzHeaderParser::read(WzBinaryReader* reader) {
    if (!reader) return Result<WzHeader>::error(ErrorCode::InvalidArgument);

    const int64_t start = reader->position();

    // First 4 bytes: signature ("PKG1" or "PKG2").
    char sig[5] = {0, 0, 0, 0, 0};
    if (reader->read(reinterpret_cast<uint8_t*>(sig), 4) != ErrorCode::Ok) {
        return Result<WzHeader>::error(ErrorCode::IOError);
    }

    WzHeader h;
    h.signature.assign(sig, 4);
    if (!h.isPkg1() && !h.isPkg2()) {
        log::error("WzHeader: unknown signature '%.4s'", sig);
        return Result<WzHeader>::error(ErrorCode::InvalidSignature);
    }

    // Next 4 bytes: version (little endian).
    h.version = reader->readI32();
    // Next 4 bytes: copyright year.
    h.copyrightYear = reader->readI32();
    // Next 44 bytes: a zero-padded string ("WzFile\0" + filler).
    uint8_t name[44] = {0};
    if (reader->read(name, 44) != ErrorCode::Ok) {
        return Result<WzHeader>::error(ErrorCode::IOError);
    }
    (void)name;

    // For PKG2 the next 16 bytes are an "IV key" used in some regions.
    // For PKG1 they are usually 16 zero bytes; we still consume them.
    if (reader->read(h.tag.data(), 16) != ErrorCode::Ok) {
        return Result<WzHeader>::error(ErrorCode::IOError);
    }

    // After the header comes the 4-byte IV. This is the first 4 bytes of
    // the encrypted region — they are *not* the IV; they are the start
    // of the encrypted directory tree. The actual IV in many KMS files
    // is implicit (0x314B) and we fall back to that when the region
    // sentinel is missing. We still read 4 bytes here for the file's
    // own use.
    uint8_t ivBuf[4] = {0x4B, 0x31, 0x00, 0x00};  // 0x314B in LE
    if (reader->read(ivBuf, 4) != ErrorCode::Ok) {
        return Result<WzHeader>::error(ErrorCode::IOError);
    }
    std::memcpy(h.iv.data(), ivBuf, 4);

    // Sanity: the version should be in a sensible range.
    if (h.version < 1 || h.version > 0xFFFF) {
        log::warn("WzHeader: version %d is out of expected range", h.version);
    }

    log::debug("WzHeader: sig=%.4s version=%d year=%d tag[0]=0x%02X",
               sig, h.version, h.copyrightYear, h.tag[0]);
    (void)start;
    return Result<WzHeader>(std::move(h));
}

}  // namespace wzlib
