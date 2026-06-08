#include "wzlib/Calculator.h"

#include "wzlib/WzBinaryReader.h"

namespace wzlib {

int32_t Calculator::compute(const uint8_t* data, size_t len) {
    return WzBinaryReader::computeChecksum(data, len);
}

int32_t Calculator::compute(const std::vector<uint8_t>& data) {
    return compute(data.data(), data.size());
}

}  // namespace wzlib
