#include "wzlib/WzNode.h"

#include "wzlib/WzImage.h"
#include "wzlib/WzObject.h"
#include "wzlib/WzUol.h"

namespace wzlib {

void WzNode::addChild(std::unique_ptr<WzNode> child) {
    if (!child) return;
    child->parent_ = this;
    children_[child->name_] = std::move(child);
}

WzNode* WzNode::child(const std::string& name) const {
    auto it = children_.find(name);
    if (it == children_.end()) return nullptr;
    return it->second.get();
}

WzNode* WzNode::get(const std::string& path) const {
    WzNode* cur = const_cast<WzNode*>(this);
    size_t i = 0;
    while (i < path.size() && cur) {
        size_t j = path.find('/', i);
        if (j == i) { ++i; continue; }  // skip empty segment
        std::string seg = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
        i = (j == std::string::npos) ? path.size() : j + 1;

        if (seg.empty() || seg == ".") continue;
        if (seg == "..") {
            cur = cur->parent_;
            continue;
        }
        auto it = cur->children_.find(seg);
        if (it == cur->children_.end()) return nullptr;
        cur = it->second.get();

        // Auto-resolve UOL.
        if (cur && cur->value_ && cur->value_->type() == ObjectType::Uol) {
            auto* uol = static_cast<WzUol*>(cur->value_.get());
            cur = uol->resolve(cur);
        }
    }
    return cur;
}

WzNode* WzNode::getFromRoot(const std::string& path) const {
    return root()->get(path);
}

WzNode* WzNode::root() const {
    WzNode* cur = const_cast<WzNode*>(this);
    while (cur->parent_) cur = cur->parent_;
    return cur;
}

std::string WzNode::fullPath() const {
    if (!parent_) return "";
    std::vector<std::string> segs;
    for (const WzNode* n = this; n && n->parent_; n = n->parent_) {
        segs.push_back(n->name_);
    }
    std::string out;
    for (auto it = segs.rbegin(); it != segs.rend(); ++it) {
        if (!out.empty()) out += '/';
        out += *it;
    }
    return out;
}

WzNode* WzNode::topEntry() const {
    // Walk up until we find a node whose parent is a WzImage root.
    const WzNode* prev = nullptr;
    const WzNode* n = this;
    while (n) {
        if (prev && n->owningImage_ && n->parent_ == nullptr) {
            return const_cast<WzNode*>(n);
        }
        prev = n;
        n = n->parent_;
    }
    return nullptr;
}

// --- Type-safe accessors ---------------------------------------------------

std::optional<int32_t> WzNode::getInt() const {
    return value_ ? value_->tryInt() : std::nullopt;
}
std::optional<int64_t> WzNode::getLong() const {
    if (!value_) return std::nullopt;
    if (auto v = value_->tryInt()) return static_cast<int64_t>(*v);
    return std::nullopt;
}
std::optional<double> WzNode::getDouble() const {
    return value_ ? value_->tryDouble() : std::nullopt;
}
std::optional<std::string> WzNode::getString() const {
    return value_ ? value_->tryString() : std::nullopt;
}

int32_t WzNode::getInt(int32_t defaultValue) const {
    auto v = getInt();
    return v ? *v : defaultValue;
}
int64_t WzNode::getLong(int64_t defaultValue) const {
    auto v = getLong();
    return v ? *v : defaultValue;
}
double WzNode::getDouble(double defaultValue) const {
    auto v = getDouble();
    return v ? *v : defaultValue;
}
std::string WzNode::getString(const std::string& defaultValue) const {
    auto v = getString();
    return v ? *v : defaultValue;
}

int32_t WzNode::getInt(const std::string& path, int32_t defaultValue) const {
    WzNode* n = get(path);
    return n ? n->getInt(defaultValue) : defaultValue;
}
std::string WzNode::getString(const std::string& path, const std::string& defaultValue) const {
    WzNode* n = get(path);
    return n ? n->getString(defaultValue) : defaultValue;
}
bool WzNode::getBool(const std::string& path) const {
    WzNode* n = get(path);
    return n && n->getInt(0) != 0;
}

}  // namespace wzlib
