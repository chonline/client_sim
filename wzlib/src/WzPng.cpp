#include "wzlib/WzPng.h"

#include "wzlib/Log.h"
#include "wzlib/PngDecoder.h"

namespace wzlib {

const char* WzPng::formatName(Format fmt) {
    switch (fmt) {
        case Format::ARGB4444:    return "ARGB4444";
        case Format::ARGB1555:    return "ARGB1555";
        case Format::RGB565:      return "RGB565";
        case Format::ARGB8888:    return "ARGB8888";
        case Format::DXT1:        return "DXT1";
        case Format::DXT3:        return "DXT3";
        case Format::DXT5:        return "DXT5";
        case Format::BC7:         return "BC7";
        case Format::RGBA1010102: return "RGBA1010102";
        case Format::R16:         return "R16";
        case Format::A8:          return "A8";
        case Format::RGBA32Float: return "RGBA32Float";
        case Format::Unknown:     return "Unknown";
    }
    return "?";
}

const std::vector<uint8_t>& WzPng::decodeRgba() const {
    if (!decoded_.empty()) {
        return decoded_;
    }

    const size_t expected = static_cast<size_t>(width_) * height_ * 4;
    if (width_ <= 0 || height_ <= 0) {
        return decoded_;
    }
    decoded_.resize(expected, 0);  // Pre-fill with transparent black.

    bool ok = false;
    const uint8_t* src = raw_.data();
    const size_t srcLen = raw_.size();
    uint8_t* dst = decoded_.data();

    switch (format_) {
        case Format::ARGB4444: ok = png::decodeARGB4444(src, srcLen, width_, height_, dst, expected); break;
        case Format::ARGB1555: ok = png::decodeARGB1555(src, srcLen, width_, height_, dst, expected); break;
        case Format::RGB565:   ok = png::decodeRGB565  (src, srcLen, width_, height_, dst, expected); break;
        case Format::ARGB8888: ok = png::decodeARGB8888(src, srcLen, width_, height_, dst, expected); break;
        case Format::DXT1:     ok = png::decodeDXT1    (src, srcLen, width_, height_, dst, expected); break;
        case Format::DXT3:     ok = png::decodeDXT3    (src, srcLen, width_, height_, dst, expected); break;
        case Format::DXT5:     ok = png::decodeDXT5    (src, srcLen, width_, height_, dst, expected); break;
        case Format::A8:       ok = png::decodeA8      (src, srcLen, width_, height_, dst, expected); break;

        case Format::BC7:
        case Format::RGBA1010102:
        case Format::R16:
        case Format::RGBA32Float:
        case Format::Unknown:
        default:
            log::warn("WzPng: unsupported format=%s (%dx%d)",
                      formatName(format_), width_, height_);
            decoded_.clear();
            return decoded_;
    }

    if (!ok) {
        log::warn("WzPng: decode failed for format=%s %dx%d",
                  formatName(format_), width_, height_);
        decoded_.clear();
    }

    return decoded_;
}

}  // namespace wzlib
