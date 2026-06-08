#include "wzlib/StringLinker.h"

#include "wzlib/Log.h"
#include "wzlib/WzImage.h"
#include "wzlib/WzNode.h"
#include "wzlib/WzString.h"

namespace wzlib {

VoidResult StringLinker::loadFromImage(const WzImage* img) {
    if (!img) return VoidResult{ErrorCode::InvalidArgument};

    WzNode* root = const_cast<WzImage*>(img)->rootNode();
    if (!root) return VoidResult{ErrorCode::NotInitialized};

    strings_.clear();
    strings_.reserve(1024);

    // Walk the children and collect their string values in index order.
    // The WZ property tree for Property.img is a list of (idx -> str)
    // pairs; we re-key it by index.
    for (auto& [name, child] : root->children()) {
        (void)name;
        const WzObject* v = child->value();
        if (!v) continue;
        if (auto* s = v->asString()) {
            strings_.push_back(s->value());
        }
    }
    log::debug("StringLinker: loaded %zu strings", strings_.size());
    return VoidResult{};
}

void StringLinker::setStrings(std::vector<std::string> strings) {
    strings_ = std::move(strings);
}

std::string StringLinker::resolve(int32_t index) const {
    if (index < 0 || index >= static_cast<int32_t>(strings_.size())) {
        return {};
    }
    return strings_[index];
}

}  // namespace wzlib
