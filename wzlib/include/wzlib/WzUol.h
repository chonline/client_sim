#pragma once

#include <string>

#include "wzlib/WzObject.h"

namespace wzlib {

class WzNode;

/**
 * @brief Symbol link (UOL = "Useless Object Link" in WZ parlance).
 *
 * A UOL stores a relative path like "../otherNode" or "Map/0/100.img" and
 * resolves to the target node on access. Resolution is implemented lazily by
 * WzNode::get(); this class just holds the path string.
 *
 * UOL paths are relative to the *containing entry's parent*, not to the UOL
 * node itself, so resolution is context-dependent.
 */
class WzUol : public WzObject {
public:
    explicit WzUol(std::string path) : path_(std::move(path)) {}
    ObjectType type() const override { return ObjectType::Uol; }
    const WzUol* asUol() const override { return this; }

    const std::string& path() const { return path_; }

    /**
     * @brief Resolve this UOL to the actual node.
     *
     * @param context  The WzNode that *contains* this UOL (used as the base
     *                 for relative path resolution). Must be non-null.
     * @return         The resolved node, or nullptr on failure.
     */
    WzNode* resolve(WzNode* context) const;

private:
    std::string path_;
};

}  // namespace wzlib
