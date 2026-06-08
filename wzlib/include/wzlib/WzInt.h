#pragma once

#include <cstdint>

#include "wzlib/WzObject.h"

namespace wzlib {

/**
 * @brief 32-bit signed integer value.
 */
class WzInt : public WzObject {
public:
    explicit WzInt(int32_t v) : value_(v) {}
    ObjectType type() const override { return ObjectType::Int; }
    const WzInt* asInt() const override { return this; }

    int32_t value() const { return value_; }
    void setValue(int32_t v) { value_ = v; }

private:
    int32_t value_;
};

}  // namespace wzlib
