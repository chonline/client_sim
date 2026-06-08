#include "ImageViewer.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QPixmap>
#include <QWheelEvent>

namespace client_sim::ui {

ImageViewer::ImageViewer(QWidget* parent)
    : QGraphicsView(parent),
      scene_(new QGraphicsScene(this)) {
    setScene(scene_);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setBackgroundBrush(QColor(40, 40, 40));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

ImageViewer::~ImageViewer() = default;

void ImageViewer::setImage(QImage img) {
    image_ = std::move(img);
    if (scene_->items().isEmpty()) {
        item_ = scene_->addPixmap(QPixmap::fromImage(image_));
    } else {
        item_->setPixmap(QPixmap::fromImage(image_));
    }
    scene_->setSceneRect(item_->boundingRect());
    resetZoom();
}

void ImageViewer::clearImage() {
    if (item_) item_->setPixmap(QPixmap{});
    image_ = QImage{};
    scene_->setSceneRect(QRectF{});
}

QSize ImageViewer::currentImageSize() const {
    return image_.size();
}

void ImageViewer::resetZoom() {
    resetTransform();
    zoom_ = 1.0;
    if (item_) centerOn(item_);
}

void ImageViewer::applyZoom(qreal factor) {
    const qreal next = zoom_ * factor;
    if (next < kMinZoom || next > kMaxZoom) {
        return;  // Clamp: do not change zoom_ either, so consecutive
                 // wheel events at the limit do not "stuck" the view.
    }
    scale(factor, factor);
    zoom_ = next;
}

void ImageViewer::wheelEvent(QWheelEvent* e) {
    if (e->angleDelta().y() > 0) {
        applyZoom(kZoomFactor);
    } else if (e->angleDelta().y() < 0) {
        applyZoom(1.0 / kZoomFactor);
    }
    e->accept();
}

}  // namespace client_sim::ui
