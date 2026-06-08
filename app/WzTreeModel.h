#pragma once

/**
 * @file WzTreeModel.h
 * @brief QAbstractItemModel that exposes a WzStructure as a tree.
 *
 * The model is lazy: a row's children are built only when Qt asks
 * for them via canFetchMore() / fetchMore(). That keeps the memory
 * footprint small for large WZ bundles - we never extract images
 * the user has not expanded.
 *
 * The model owns the TreeNode tree but does NOT own the
 * WzStructure; ownership of the WzStructure stays in MainWindow.
 */

#include <QAbstractItemModel>
#include <QHash>
#include <QPixmap>

#include <memory>
#include <string>
#include <vector>

namespace wzlib { class WzStructure; }
namespace wzlib { class WzFile; }
namespace wzlib { class WzDirectory; }
namespace wzlib { class WzImage; }
namespace wzlib { class WzNode; }
namespace wzlib { class WzPng; }

namespace client_sim::ui {

struct TreeNode {
    enum class Kind { WzFile, WzDirectory, WzImage, WzNode };
    Kind kind = Kind::WzDirectory;
    wzlib::WzFile*       file = nullptr;
    wzlib::WzDirectory*  dir  = nullptr;
    wzlib::WzImage*      img  = nullptr;
    wzlib::WzNode*       node = nullptr;
    std::string          path;     // slash-joined path under the WzFile
    bool                 expanded = false;
    std::vector<std::unique_ptr<TreeNode>> children;
};

class WzTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit WzTreeModel(QObject* parent = nullptr);
    ~WzTreeModel() override;

    /// Replace the underlying structure. The model does not take
    /// ownership; the previous structure (if any) must be destroyed
    /// by its owner.
    void setStructure(wzlib::WzStructure* s);
    wzlib::WzStructure* structure() const noexcept { return structure_; }

    /// Drop all rows and clear the thumbnail cache. Safe to call
    /// when no structure is loaded.
    void reset();

    // -- QAbstractItemModel --
    QModelIndex index(int row, int column,
                      const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool hasChildren(const QModelIndex& parent) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // -- Helpers used by WzTreeView --
    TreeNode* nodeForIndex(const QModelIndex& idx) const;
    QString   fullPath(const QModelIndex& idx) const;

    /// Custom role: ObjectType as int, or -1 for non-value rows.
    static constexpr int TypeRole = Qt::UserRole + 1;

private:
    wzlib::WzStructure* structure_ = nullptr;
    std::unique_ptr<TreeNode> invisibleRoot_;

    // 16x16 thumbnails keyed by WzPng* (stable for the WzStructure's
    // lifetime). Capped to kMaxThumbCache entries.
    mutable QHash<const wzlib::WzPng*, QPixmap> thumbCache_;
    static constexpr int kMaxThumbCache = 256;

    void buildWzFileChildren(TreeNode* fileNode);
    void buildDirectoryChildren(TreeNode* dirNode);
    void buildImageChildren(TreeNode* imgNode);
    void buildWzNodeChildren(TreeNode* nodeRow);
    QPixmap makeThumbnail(const wzlib::WzPng* png) const;
    void evictThumbnailsIfFull();
};

}  // namespace client_sim::ui
