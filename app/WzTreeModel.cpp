#include "WzTreeModel.h"

#include <QImage>
#include <QString>

#include <algorithm>
#include <functional>

#include "RgbaToPixmap.h"

#include "wzlib/WzDirectory.h"
#include "wzlib/WzFile.h"
#include "wzlib/WzImage.h"
#include "wzlib/WzNode.h"
#include "wzlib/WzObject.h"
#include "wzlib/WzPng.h"
#include "wzlib/WzStructure.h"

namespace client_sim::ui {

namespace {

QString qs(const std::string& s) { return QString::fromStdString(s); }

// Returns a short type label for column 1 of a leaf value row.
QString typeLabel(const wzlib::WzObject* v) {
    if (!v) return {};
    switch (v->type()) {
        case wzlib::ObjectType::Int:    return QStringLiteral("Int");
        case wzlib::ObjectType::Long:   return QStringLiteral("Long");
        case wzlib::ObjectType::Float:  return QStringLiteral("Float");
        case wzlib::ObjectType::Double: return QStringLiteral("Double");
        case wzlib::ObjectType::String: return QStringLiteral("String");
        case wzlib::ObjectType::Uol:    return QStringLiteral("Uol");
        case wzlib::ObjectType::Png:    return QStringLiteral("Png");
        case wzlib::ObjectType::Sound:  return QStringLiteral("Sound");
        case wzlib::ObjectType::Vector2D: return QStringLiteral("Vector");
        case wzlib::ObjectType::Convex: return QStringLiteral("Convex");
        case wzlib::ObjectType::RawData: return QStringLiteral("Raw");
        case wzlib::ObjectType::Video:  return QStringLiteral("Video");
        case wzlib::ObjectType::Property: return QStringLiteral("Property");
        default: return QStringLiteral("?");
    }
}

}  // namespace

WzTreeModel::WzTreeModel(QObject* parent)
    : QAbstractItemModel(parent),
      invisibleRoot_(std::make_unique<TreeNode>()) {
    invisibleRoot_->kind = TreeNode::Kind::WzFile;  // sentinel
}

WzTreeModel::~WzTreeModel() = default;

void WzTreeModel::setStructure(wzlib::WzStructure* s) {
    beginResetModel();
    structure_ = s;
    invisibleRoot_ = std::make_unique<TreeNode>();
    invisibleRoot_->kind = TreeNode::Kind::WzFile;
    thumbCache_.clear();
    if (s) {
        for (const auto& f : s->files()) {
            auto fileRow = std::make_unique<TreeNode>();
            fileRow->kind = TreeNode::Kind::WzFile;
            fileRow->file = f.get();
            fileRow->path = f->path();
            invisibleRoot_->children.push_back(std::move(fileRow));
        }
    }
    endResetModel();
}

void WzTreeModel::reset() {
    setStructure(nullptr);
}

QModelIndex WzTreeModel::index(int row, int column,
                                const QModelIndex& parent) const {
    if (column < 0 || row < 0 || !invisibleRoot_) {
        return {};
    }
    TreeNode* parentNode = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : invisibleRoot_.get();
    if (row >= static_cast<int>(parentNode->children.size())) {
        return {};
    }
    return createIndex(row, column, parentNode->children[row].get());
}

QModelIndex WzTreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid()) return {};
    TreeNode* child = static_cast<TreeNode*>(index.internalPointer());
    // Walk up: we don't keep a parent pointer in TreeNode, so we
    // search invisibleRoot_'s descendants. This is O(N) per index()
    // call. Acceptable for M1's tree sizes; if it ever shows up in
    // a profile, add a TreeNode* parent_ back-pointer.
    if (!child) return {};
    // Linear search: find the TreeNode* that owns `child` as a child.
    std::function<TreeNode*(TreeNode*, TreeNode*)> findOwner =
        [&](TreeNode* owner, TreeNode* target) -> TreeNode* {
        for (auto& c : owner->children) {
            if (c.get() == target) return owner;
            TreeNode* r = findOwner(c.get(), target);
            if (r) return r;
        }
        return nullptr;
    };
    TreeNode* owner = findOwner(invisibleRoot_.get(), child);
    if (!owner || owner == invisibleRoot_.get()) {
        return {};
    }
    // Find owner's index among its siblings.
    TreeNode* grand = findOwner(invisibleRoot_.get(), owner);
    if (!grand) return {};
    auto it = std::find_if(grand->children.begin(), grand->children.end(),
        [&](const std::unique_ptr<TreeNode>& n){ return n.get() == owner; });
    if (it == grand->children.end()) return {};
    return createIndex(static_cast<int>(it - grand->children.begin()),
                       0, owner);
}

int WzTreeModel::rowCount(const QModelIndex& parent) const {
    if (!invisibleRoot_) return 0;
    TreeNode* p = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : invisibleRoot_.get();
    return static_cast<int>(p->children.size());
}

int WzTreeModel::columnCount(const QModelIndex&) const {
    return 2;
}

QVariant WzTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    TreeNode* n = static_cast<TreeNode*>(index.internalPointer());
    if (!n) return {};

    switch (role) {
        case Qt::DisplayRole:
            if (index.column() == 0) {
                if (n->kind == TreeNode::Kind::WzFile) {
                    // Show basename of the file path, not the full path.
                    const std::string& p = n->path;
                    auto pos = p.find_last_of("/\\");
                    return qs(pos == std::string::npos ? p : p.substr(pos + 1));
                }
                if (n->kind == TreeNode::Kind::WzImage) return qs(n->dir->name());
                if (n->kind == TreeNode::Kind::WzDirectory) return qs(n->dir->name());
                return qs(n->node->name());
            } else if (index.column() == 1) {
                if (n->kind == TreeNode::Kind::WzNode && n->node->value()) {
                    return typeLabel(n->node->value());
                }
                if (n->kind == TreeNode::Kind::WzImage) {
                    return n->img && n->img->extracted()
                        ? QStringLiteral("Image")
                        : QStringLiteral("Image (lazy)");
                }
            }
            return {};

        case Qt::DecorationRole:
            if (index.column() == 0) {
                if (n->kind == TreeNode::Kind::WzFile) {
                    return QPixmap(QStringLiteral(":/icons/folder.png"));
                }
                if (n->kind == TreeNode::Kind::WzNode && n->node->value()) {
                    if (auto* png = n->node->value()->asPng()) {
                        if (!thumbCache_.contains(png)) {
                            thumbCache_.insert(png, makeThumbnail(png));
                            evictThumbnailsIfFull();
                        }
                        return thumbCache_.value(png);
                    }
                }
            }
            return {};

        case Qt::ToolTipRole: {
            if (n->kind == TreeNode::Kind::WzFile) return qs(n->path);
            return qs(n->path);
        }

        case TypeRole:
            if (n->kind == TreeNode::Kind::WzNode && n->node->value()) {
                return static_cast<int>(n->node->value()->type());
            }
            return -1;
    }
    return {};
}

bool WzTreeModel::hasChildren(const QModelIndex& parent) const {
    TreeNode* n = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : invisibleRoot_.get();
    if (!n) return false;
    switch (n->kind) {
        case TreeNode::Kind::WzFile:       return !n->file->root()->children().empty();
        case TreeNode::Kind::WzDirectory:  return n->dir->childCount() > 0;
        case TreeNode::Kind::WzImage:      return true;  // sub-properties may exist
        case TreeNode::Kind::WzNode:       return n->node->childCount() > 0;
    }
    return false;
}

bool WzTreeModel::canFetchMore(const QModelIndex& parent) const {
    TreeNode* n = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : invisibleRoot_.get();
    return n && !n->expanded;
}

void WzTreeModel::fetchMore(const QModelIndex& parent) {
    TreeNode* n = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : invisibleRoot_.get();
    if (!n || n->expanded) return;

    beginInsertRows(parent, 0,
        std::max(0, static_cast<int>(n->children.size()) - 1));

    switch (n->kind) {
        case TreeNode::Kind::WzFile:      buildWzFileChildren(n);     break;
        case TreeNode::Kind::WzDirectory: buildDirectoryChildren(n);  break;
        case TreeNode::Kind::WzImage:     buildImageChildren(n);      break;
        case TreeNode::Kind::WzNode:      buildWzNodeChildren(n);     break;
    }
    n->expanded = true;

    endInsertRows();
}

TreeNode* WzTreeModel::nodeForIndex(const QModelIndex& idx) const {
    return idx.isValid() ? static_cast<TreeNode*>(idx.internalPointer()) : nullptr;
}

QString WzTreeModel::fullPath(const QModelIndex& idx) const {
    TreeNode* n = nodeForIndex(idx);
    return n ? qs(n->path) : QString{};
}

void WzTreeModel::buildWzFileChildren(TreeNode* fileNode) {
    if (!fileNode->file || !fileNode->file->root()) return;
    for (const auto& childDir : fileNode->file->root()->children()) {
        auto row = std::make_unique<TreeNode>();
        row->kind = TreeNode::Kind::WzDirectory;
        row->file = fileNode->file;
        row->dir  = childDir.get();
        row->path = childDir->name();
        fileNode->children.push_back(std::move(row));
    }
}

void WzTreeModel::buildDirectoryChildren(TreeNode* dirNode) {
    if (!dirNode->dir) return;
    // Directories whose children are all WzImages (i.e. .img leaves)
    // become WzImage rows. We detect a "leaf directory" by
    // childCount() == 0 in wzlib's WzDirectory (which is set when
    // the on-disk child is an image, not another directory).
    // For safety we treat every direct child of the WzDirectory as
    // either a WzDirectory or a WzImage by checking whether its
    // size() field is non-zero (size == data block size = image).
    for (const auto& childDir : dirNode->dir->children()) {
        auto row = std::make_unique<TreeNode>();
        if (childDir->size() > 0) {
            // Leaf: a WzImage.
            row->kind = TreeNode::Kind::WzImage;
            row->img  = dirNode->file->findImage(childDir->name());
            row->dir  = childDir.get();
            row->path = dirNode->path + "/" + childDir->name();
        } else {
            row->kind = TreeNode::Kind::WzDirectory;
            row->dir  = childDir.get();
            row->path = dirNode->path + "/" + childDir->name();
        }
        row->file = dirNode->file;
        dirNode->children.push_back(std::move(row));
    }
}

void WzTreeModel::buildImageChildren(TreeNode* imgNode) {
    if (!imgNode->img) return;
    if (!imgNode->img->extracted()) {
        // tryExtract is the responsibility of WzTreeView (it owns
        // the wait cursor + status bar flow). When this is reached
        // for an un-extracted image, the row will simply have no
        // children until the view triggers extraction and re-asks
        // for fetchMore().
        return;
    }
    const wzlib::WzNode* root = imgNode->img->rootNode();
    for (const auto& [name, child] : root->children()) {
        auto row = std::make_unique<TreeNode>();
        row->kind = TreeNode::Kind::WzNode;
        row->file = imgNode->file;
        row->img  = imgNode->img;
        row->node = child.get();
        row->path = imgNode->path + "/" + name;
        imgNode->children.push_back(std::move(row));
    }
}

void WzTreeModel::buildWzNodeChildren(TreeNode* nodeRow) {
    if (!nodeRow->node) return;
    for (const auto& [name, child] : nodeRow->node->children()) {
        auto row = std::make_unique<TreeNode>();
        row->kind = TreeNode::Kind::WzNode;
        row->file = nodeRow->file;
        row->img  = nodeRow->img;
        row->node = child.get();
        row->path = nodeRow->path + "/" + name;
        nodeRow->children.push_back(std::move(row));
    }
}

QPixmap WzTreeModel::makeThumbnail(const wzlib::WzPng* png) const {
    const auto& rgba = png->decodeRgba();
    if (rgba.empty()) return {};
    QImage img = rgbaToQImage(rgba.data(), png->width(), png->height());
    if (img.isNull()) return {};
    return QPixmap::fromImage(img.scaled(16, 16, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));
}

void WzTreeModel::evictThumbnailsIfFull() {
    if (thumbCache_.size() <= kMaxThumbCache) return;
    // Evict the oldest insertion: QHash::begin() yields entries in
    // insertion order in Qt 6. Remove the first one. Repeat until
    // under the cap.
    while (thumbCache_.size() > kMaxThumbCache) {
        auto it = thumbCache_.begin();
        if (it == thumbCache_.end()) break;
        thumbCache_.erase(it);
    }
}

}  // namespace client_sim::ui
