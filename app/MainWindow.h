#pragma once

/**
 * @file MainWindow.h
 * @brief Top-level QMainWindow for the client_sim WZ browser.
 *
 * Owns the WzStructure lifetime, holds the WzTreeView + ImageViewer
 * in a QSplitter, owns the menu and toolbar.
 */

#include <QMainWindow>
#include <QString>

#include <memory>

namespace wzlib { class WzStructure; }

namespace client_sim::ui {

class WzTreeView;
class ImageViewer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /// Replace the current structure. The MainWindow takes ownership.
    void setStructure(std::unique_ptr<wzlib::WzStructure> s,
                      const QString& sourcePath);

    /// Drop the current structure and reset the UI to empty state.
    void closeStructure();

    bool hasStructure() const noexcept { return structure_ != nullptr; }

private slots:
    void onOpenDirectory();
    void onResetZoom();
    void onAbout();
    void onImageActivated(const wzlib::WzPng* png, int width, int height,
                          const QString& formatName, const QString& path);
    void onNonImageSelected(const QString& path);

private:
    std::unique_ptr<wzlib::WzStructure> structure_;
    QString sourcePath_;

    WzTreeView*  treeView_  = nullptr;
    ImageViewer* imageView_ = nullptr;
    QAction*     actOpenDir_ = nullptr;
    QAction*     actReset_   = nullptr;
    QAction*     actExit_    = nullptr;
    QAction*     actAbout_   = nullptr;

    void buildMenusAndToolbar();
    void wireSignals();
};

}  // namespace client_sim::ui
