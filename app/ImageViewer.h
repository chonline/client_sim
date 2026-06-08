#pragma once

/**
 * @file ImageViewer.h
 * @brief QGraphicsView-based image canvas with wheel-zoom and drag-pan.
 *
 * The viewer is a passive display surface: it does not know about
 * WzPng or WzStructure. The owner (MainWindow) calls setImage() with
 * a ready-to-display QImage. Multi-frame animation, sprite playback,
 * and Spine are out of scope for M1.
 */

#include <QGraphicsView>
#include <QImage>

class QGraphicsPixmapItem;
class QGraphicsScene;
class QWheelEvent;

namespace client_sim::ui {

class ImageViewer : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageViewer(QWidget* parent = nullptr);
    ~ImageViewer() override;

    /// Replace the current image. The previous image is discarded.
    void setImage(QImage img);

    /// Remove the current image and reset the scene to empty.
    void clearImage();

    /// Returns (0, 0) when no image is loaded.
    QSize currentImageSize() const;

    bool hasImage() const noexcept { return !image_.isNull(); }

public slots:
    /// Reset zoom to 1.0x and center the image.
    void resetZoom();

private:
    QGraphicsScene*        scene_ = nullptr;
    QGraphicsPixmapItem*   item_  = nullptr;
    QImage                 image_;
    qreal                  zoom_  = 1.0;

    static constexpr qreal kMinZoom     = 0.05;
    static constexpr qreal kMaxZoom     = 20.0;
    static constexpr qreal kZoomFactor  = 1.15;

    void applyZoom(qreal factor);
    void wheelEvent(QWheelEvent* e) override;
};

}  // namespace client_sim::ui
