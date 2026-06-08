#pragma once

#include "wzlib/WzObject.h"

namespace wzlib {

/**
 * @brief IEEE 754 double-precision floating point value.
 */
class WzDouble : public WzObject {
public:
    explicit WzDouble(double v) : value_(v) {}
    ObjectType type() const override { return ObjectType::Double; }
    const WzDouble* asDouble() const override { return this; }

    double value() const { return value_; }

private:
    double value_;
};

}  // namespace wzlib
