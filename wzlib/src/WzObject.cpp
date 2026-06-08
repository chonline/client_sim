#include "wzlib/WzObject.h"

#include "wzlib/WzInt.h"
#include "wzlib/WzDouble.h"
#include "wzlib/WzString.h"
#include "wzlib/WzVector.h"

namespace wzlib {

std::optional<int32_t> WzObject::tryInt() const {
    if (auto* p = asInt()) return p->value();
    return std::nullopt;
}

std::optional<double> WzObject::tryDouble() const {
    if (auto* p = asDouble()) return p->value();
    if (auto* p = asInt())    return static_cast<double>(p->value());
    return std::nullopt;
}

std::optional<std::string> WzObject::tryString() const {
    if (auto* p = asString()) return p->value();
    return std::nullopt;
}

}  // namespace wzlib
