#pragma once

#include <cstdint>

#include "wzlib/WzObject.h"

namespace wzlib {

/**
 * @brief 2D integer vector (used for sprite origins, foothold endpoints, etc.).
 */
class WzVector : public WzObject {
public:
    WzVector(int32_t x, int32_t y) : x_(x), y_(y) {}
    WzVector() = default;
    ObjectType type() const override { return ObjectType::Vector2D; }
    const WzVector* asVector() const override { return this; }

    int32_t x() const { return x_; }
    int32_t y() const { return y_; }

    void setX(int32_t v) { x_ = v; }
    void setY(int32_t v) { y_ = v; }

private:
    int32_t x_ = 0;
    int32_t y_ = 0;
};

}  // namespace wzlib
