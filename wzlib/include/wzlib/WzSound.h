#pragma once

#include <cstdint>
#include <vector>

#include "wzlib/WzObject.h"

namespace wzlib {

/**
 * @brief Audio data (MP3 or raw PCM).
 *
 * The raw bytes are stored decrypted and decompressed — to play them, hand
 * the data to your platform's audio decoder (e.g. SDL2_mixer).
 */
class WzSound : public WzObject {
public:
    enum class Type : uint8_t {
        MP3 = 0,   ///< MPEG-1 Audio Layer III bitstream.
        PCM = 1,   ///< Raw PCM samples (WAV-like, no header).
    };

    WzSound(Type t, int32_t durationMs, std::vector<uint8_t> data)
        : type_(t), durationMs_(durationMs), data_(std::move(data)) {}

    ObjectType type() const override { return ObjectType::Sound; }
    const WzSound* asSound() const override { return this; }

    Type soundType() const { return type_; }
    int32_t durationMs() const { return durationMs_; }
    const std::vector<uint8_t>& data() const { return data_; }

private:
    Type type_;
    int32_t durationMs_;
    std::vector<uint8_t> data_;
};

}  // namespace wzlib
