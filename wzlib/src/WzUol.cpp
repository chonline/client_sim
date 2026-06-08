#include "wzlib/WzUol.h"

#include "wzlib/WzNode.h"

namespace wzlib {

WzNode* WzUol::resolve(WzNode* context) const {
    if (!context) return nullptr;

    // UOL paths are relative to the parent of the *containing entry*, not
    // the UOL node itself. See the original C# implementation in
    // WzComparerR2.WzLib/Wz_Uol.cs for the historical context.
    WzNode* base = context->parent();
    if (!base) {
        // UOL at root level — try the context itself.
        base = context;
    }

    if (path_.empty()) return nullptr;
    if (path_[0] == '/') {
        // Absolute path: from root.
        return base->getFromRoot(path_.substr(1));
    }
    return base->get(path_);
}

}  // namespace wzlib
