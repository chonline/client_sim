#pragma once

/**
 * @file Calculator.h
 * @brief Pre-compute the WZ CRC-32 table used for image integrity checks.
 *
 * The WZ "checksum" (a 32-bit value stored alongside every WZImage) is
 * the standard CRC-32 with polynomial 0xEDB88320. WzBinaryReader does
 * the actual computation; Calculator just exposes a one-shot helper
 * for batch / offline use.
 */

#include <cstdint>
#include <cstddef>
#include <vector>

namespace wzlib {

class Calculator {
public:
    /**
     * @brief Compute the WZ CRC-32 over @p data.
     */
    static int32_t compute(const uint8_t* data, size_t len);

    /**
     * @brief Convenience: compute over a vector.
     */
    static int32_t compute(const std::vector<uint8_t>& data);
};

}  // namespace wzlib
