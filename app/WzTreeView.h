#pragma once

/**
 * @file WzTreeView.h
 * @brief QTreeView subclass that triggers lazy WzImage extraction.
 *
 * When the user expands an un-extracted WzImage row, this view
 * calls WzImage::tryExtract under a Qt::WaitCursor and refreshes
 * the model. When the user activates a row, the view emits
 * either imageActivated() (for rows whose value is a WzPng) or
 * nonImageSelected() (for everything else).
 */

#include <QTreeView>

namespace wzlib { class WzPng; }

namespace client_sim::ui {

class WzTreeModel;

class WzTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit WzTreeView(QWidget* parent = nullptr);
    ~WzTreeView() override;

    void setModel(QAbstractItemModel* m) override;

    /// Force-extract a specific image (no-op if already extracted).
    /// Returns true on success or if the image was already extracted.
    bool ensureExtracted(wzlib::WzImage* img);

signals:
    void imageActivated(const wzlib::WzPng* png, int width, int height,
                        const QString& formatName, const QString& path);
    void nonImageSelected(const QString& path);

private slots:
    void onExpanded(const QModelIndex& idx);
    void onActivatedSlot(const QModelIndex& idx);

private:
    QString pathFor(const QModelIndex& idx) const;
};

}  // namespace client_sim::ui
