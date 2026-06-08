#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>

#include "ImageViewer.h"
#include "RgbaToPixmap.h"
#include "WzTreeModel.h"
#include "WzTreeView.h"

#include "wzlib/ErrorCode.h"
#include "wzlib/Log.h"
#include "wzlib/WzPng.h"
#include "wzlib/WzStructure.h"

namespace client_sim::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("client_sim"));
    resize(1200, 800);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    treeView_  = new WzTreeView(splitter);
    imageView_ = new ImageViewer(splitter);
    splitter->addWidget(treeView_);
    splitter->addWidget(imageView_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    setCentralWidget(splitter);

    statusBar()->showMessage(tr("Ready. File -> Open Directory to begin."));

    buildMenusAndToolbar();
    wireSignals();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildMenusAndToolbar() {
    actOpenDir_ = new QAction(tr("&Open Directory..."), this);
    actOpenDir_->setShortcut(QKeySequence::Open);
    connect(actOpenDir_, &QAction::triggered,
            this, &MainWindow::onOpenDirectory);

    actExit_ = new QAction(tr("E&xit"), this);
    actExit_->setShortcut(QKeySequence::Quit);
    connect(actExit_, &QAction::triggered, this, &QMainWindow::close);

    actReset_ = new QAction(tr("&Reset Zoom"), this);
    actReset_->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(actReset_, &QAction::triggered,
            imageView_, &ImageViewer::resetZoom);

    actAbout_ = new QAction(tr("&About client_sim"), this);
    connect(actAbout_, &QAction::triggered, this, &MainWindow::onAbout);

    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actOpenDir_);
    fileMenu->addSeparator();
    fileMenu->addAction(actExit_);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(actReset_);

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(actAbout_);

    auto* tb = addToolBar(tr("Main"));
    tb->setObjectName(QStringLiteral("MainToolBar"));
    tb->addAction(actOpenDir_);
    tb->addAction(actReset_);
}

void MainWindow::wireSignals() {
    connect(treeView_, &WzTreeView::imageActivated,
            this, &MainWindow::onImageActivated);
    connect(treeView_, &WzTreeView::nonImageSelected,
            this, &MainWindow::onNonImageSelected);
}

void MainWindow::onOpenDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open WZ Directory"));
    if (dir.isEmpty()) return;

    auto result = wzlib::WzStructure::open(dir.toStdString());
    if (result.isError()) {
        QMessageBox::warning(this, tr("Open failed"),
            tr("Could not open WZ directory:\n%1")
                .arg(QString::fromStdString(
                    wzlib::toString(result.error()))));
        return;
    }
    auto* rawPtr = result.value().get();
    setStructure(std::move(result).value(), dir);
    statusBar()->showMessage(
        tr("Loaded %1 files from %2").arg(rawPtr->fileCount()).arg(dir));
}

void MainWindow::setStructure(std::unique_ptr<wzlib::WzStructure> s,
                              const QString& sourcePath) {
    structure_ = std::move(s);
    sourcePath_ = sourcePath;
    auto* m = qobject_cast<WzTreeModel*>(treeView_->model());
    if (!m) {
        m = new WzTreeModel(this);
        treeView_->setModel(m);
    }
    m->setStructure(structure_.get());
    setWindowTitle(tr("client_sim - %1")
        .arg(QFileInfo(sourcePath).fileName()));
    imageView_->clearImage();
}

void MainWindow::closeStructure() {
    auto* m = qobject_cast<WzTreeModel*>(treeView_->model());
    if (m) m->reset();
    structure_.reset();
    sourcePath_.clear();
    setWindowTitle(tr("client_sim"));
    imageView_->clearImage();
    statusBar()->showMessage(tr("Ready."));
}

void MainWindow::onResetZoom() {
    imageView_->resetZoom();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About client_sim"),
        tr("client_sim - WZ resource browser\n"
           "Built on wzlib (commit %1)\n"
           "Qt6 / C++17").arg(QStringLiteral("M1 finish")));
}

void MainWindow::onImageActivated(const wzlib::WzPng* png, int width,
                                  int height, const QString& formatName,
                                  const QString& path) {
    if (!png) {
        onNonImageSelected(path);
        return;
    }
    const auto& rgba = png->decodeRgba();
    if (rgba.empty()) {
        statusBar()->showMessage(
            tr("Decode failed: %1").arg(path));
        imageView_->clearImage();
        return;
    }
    QImage img = rgbaToQImage(rgba.data(), width, height);
    if (img.isNull()) {
        statusBar()->showMessage(
            tr("Image conversion failed: %1").arg(path));
        imageView_->clearImage();
        return;
    }
    imageView_->setImage(img);
    statusBar()->showMessage(
        tr("%1 - %2x%3  %4").arg(path).arg(width).arg(height).arg(formatName));
}

void MainWindow::onNonImageSelected(const QString& path) {
    imageView_->clearImage();
    statusBar()->showMessage(path);
}

}  // namespace client_sim::ui
