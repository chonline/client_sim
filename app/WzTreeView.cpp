#include "WzTreeView.h"

#include <QApplication>
#include <QCursor>
#include <QModelIndex>
#include <QString>

#include "WzTreeModel.h"

#include "wzlib/Log.h"
#include "wzlib/WzBinaryReader.h"
#include "wzlib/WzCrypto.h"
#include "wzlib/WzFile.h"
#include "wzlib/WzImage.h"
#include "wzlib/WzNode.h"
#include "wzlib/WzObject.h"
#include "wzlib/WzPng.h"
#include "wzlib/ErrorCode.h"

namespace client_sim::ui {

WzTreeView::WzTreeView(QWidget* parent) : QTreeView(parent) {
    setUniformRowHeights(true);
    setHeaderHidden(false);
    setAnimated(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(this, &QTreeView::expanded,
            this, &WzTreeView::onExpanded);
    connect(this, &QTreeView::activated,
            this, &WzTreeView::onActivatedSlot);
    connect(this, &QTreeView::clicked,
            this, &WzTreeView::onActivatedSlot);
}

WzTreeView::~WzTreeView() = default;

void WzTreeView::setModel(QAbstractItemModel* m) {
    QTreeView::setModel(m);
    if (m) {
        // Column 0 is name, column 1 is type label.
        header()->resizeSection(0, 280);
        header()->resizeSection(1, 100);
    }
}

QString WzTreeView::pathFor(const QModelIndex& idx) const {
    auto* m = qobject_cast<WzTreeModel*>(model());
    return m ? m->fullPath(idx) : QString{};
}

bool WzTreeView::ensureExtracted(wzlib::WzImage* img) {
    if (!img) return false;
    if (img->extracted()) return true;
    if (!img->file()) return false;
    auto* reader = &img->file()->reader();
    auto* crypto = &img->file()->crypto();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto result = img->tryExtract(reader, crypto);
    QApplication::restoreOverrideCursor();
    if (result.isError()) {
        wzlib::log::warn("WzImage::tryExtract failed: %s",
                         wzlib::toString(result.error()));
        return false;
    }
    return true;
}

void WzTreeView::onExpanded(const QModelIndex& idx) {
    auto* m = qobject_cast<WzTreeModel*>(model());
    if (!m) return;
    TreeNode* tn = m->nodeForIndex(idx);
    if (!tn || tn->kind != TreeNode::Kind::WzImage) return;
    if (tn->img && !tn->img->extracted()) {
        if (ensureExtracted(tn->img)) {
            // Re-fetch children. fetchMore is idempotent; once
            // expanded_ == true, it does nothing - so we need to
            // reset the flag and ask the model to fetch again.
            tn->expanded = false;
            m->fetchMore(idx);
        }
    }
}

void WzTreeView::onActivatedSlot(const QModelIndex& idx) {
    auto* m = qobject_cast<WzTreeModel*>(model());
    if (!m) return;
    TreeNode* tn = m->nodeForIndex(idx);
    if (!tn) return;
    const QString path = pathFor(idx);

    if (tn->kind == TreeNode::Kind::WzNode && tn->node && tn->node->value()) {
        if (auto* png = tn->node->value()->asPng()) {
            // decodeRgba is cached inside WzPng; safe to call.
            const auto& rgba = png->decodeRgba();
            if (rgba.empty()) {
                emit nonImageSelected(path);
                return;
            }
            emit imageActivated(png, png->width(), png->height(),
                                QString::fromUtf8(wzlib::WzPng::formatName(
                                    png->format())),
                                path);
            return;
        }
    }
    emit nonImageSelected(path);
}

}  // namespace client_sim::ui
