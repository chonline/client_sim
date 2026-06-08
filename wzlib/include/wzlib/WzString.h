#pragma once

#include <string>
#include <utility>

#include "wzlib/WzObject.h"

namespace wzlib {

/**
 * @brief UTF-8 string value. Stored decrypted (i.e. already through WzCrypto).
 */
class WzString : public WzObject {
public:
    explicit WzString(std::string v) : value_(std::move(v)) {}
    ObjectType type() const override { return ObjectType::String; }
    const WzString* asString() const override { return this; }

    const std::string& value() const { return value_; }

private:
    std::string value_;
};

}  // namespace wzlib
