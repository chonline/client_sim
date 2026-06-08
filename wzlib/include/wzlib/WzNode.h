#pragma once

/**
 * @file WzNode.h
 * @brief Tree node for a parsed WZ file.
 *
 * The WZ file format is fundamentally a tree:
 *
 *   <root>
 *     ├─ Character/
 *     │   ├─ 00002000.img      ← WzImage (lazy-extracted data block)
 *     │   │   ├─ stand/        ← WzNode (sub-property)
 *     │   │   │   ├─ 0/        ← WzNode
 *     │   │   │   │   └─ canvas ← WzPng
 *     │   │   └─ walk/
 *     │   └─ ...
 *     └─ Map/
 *         └─ ...
 *
 * WzNode represents one node in this tree. Each node has:
 *   - a name
 *   - a parent (null only for the root)
 *   - an ordered set of child nodes (WzNode*)
 *   - an optional value (WzObject*) — when present, this is a leaf value
 *
 * The tree is constructed lazily: directory entries are read up front to
 * populate the structure, but WzImage contents are only extracted when
 * the user navigates into them.
 */

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "wzlib/WzObject.h"

namespace wzlib {

class WzImage;

/**
 * @brief A node in the WZ file tree.
 *
 * Memory ownership:
 *   - The parent owns all its children (via unique_ptr).
 *   - A node may own its value (WzObject* via unique_ptr) but values can
 *     also be shared between multiple nodes if they were intentionally
 *     deduplicated.
 */
class WzNode {
public:
    WzNode() = default;
    WzNode(std::string name, WzNode* parent = nullptr)
        : name_(std::move(name)), parent_(parent) {}

    // --- Identification -----------------------------------------------------
    const std::string& name() const { return name_; }
    WzNode* parent() const { return parent_; }
    void setParent(WzNode* p) { parent_ = p; }

    // --- Tree walking -------------------------------------------------------
    /** @brief Add a child node, taking ownership. */
    void addChild(std::unique_ptr<WzNode> child);

    /** @brief Find a direct child by name. Returns nullptr if not found. */
    WzNode* child(const std::string& name) const;

    /** @brief Read-only access to the children map. */
    const std::map<std::string, std::unique_ptr<WzNode>>& children() const {
        return children_;
    }
    int32_t childCount() const { return static_cast<int32_t>(children_.size()); }
    bool hasChildren() const { return !children_.empty(); }

    /**
     * @brief Resolve a path against this node.
     *
     * Supports:
     *   - forward slash separator:  "Map/Back/0"
     *   - parent navigation:        "../foo"
     *   - leading slash:            "/Map" (resolves against the root)
     *   - automatic UOL resolution (transparently)
     *
     * Returns nullptr if any segment is missing or a UOL cannot be resolved.
     */
    WzNode* get(const std::string& path) const;

    /** @brief Convenience: get() starting at the root, with no leading '/'. */
    WzNode* getFromRoot(const std::string& path) const;

    /**
     * @brief Walk up the parent chain to the root node.
     *
     * Returns @c this if it has no parent. Never returns null.
     */
    WzNode* root() const;

    /**
     * @brief Returns the full slash-separated path from the root.
     *
     * For the root itself, returns "".
     */
    std::string fullPath() const;

    /**
     * @brief Find the topmost ancestor that is a direct child of a WzImage.
     *
     * For nodes inside an image (.img), this returns the image root node.
     * For nodes that are not inside an image, returns nullptr.
     */
    WzNode* topEntry() const;

    // --- Value access -------------------------------------------------------
    /** @brief Set the value object, taking ownership. */
    void setValue(std::unique_ptr<WzObject> v) { value_ = std::move(v); }

    const WzObject* value() const { return value_.get(); }
    WzObject* value() { return value_.get(); }

    /** @brief Return the owning WzImage, or nullptr if not inside one. */
    WzImage* owningImage() const { return owningImage_; }
    void setOwningImage(WzImage* img) { owningImage_ = img; }

    // --- Type-safe accessors (return nullopt if value is wrong type) --------
    std::optional<int32_t>    getInt()    const;
    std::optional<int64_t>    getLong()   const;
    std::optional<double>     getDouble() const;
    std::optional<std::string> getString() const;

    // --- getInt with default (compatible with WzComparerR2 API) ------------
    int32_t    getInt(int32_t    defaultValue) const;
    int64_t    getLong(int64_t  defaultValue) const;
    double     getDouble(double defaultValue) const;
    std::string getString(const std::string& defaultValue) const;

    // --- WzComparerR2 compatibility helpers (operate on a path) -----------
    int32_t    getInt(const std::string& path, int32_t    defaultValue) const;
    std::string getString(const std::string& path, const std::string& defaultValue) const;
    bool       getBool(const std::string& path) const;

private:
    std::string name_;
    WzNode* parent_ = nullptr;
    std::map<std::string, std::unique_ptr<WzNode>> children_;
    std::unique_ptr<WzObject> value_;
    WzImage* owningImage_ = nullptr;

    // Allow WzImage to set owningImage_ on children.
    friend class WzImage;
};

}  // namespace wzlib
