#include "RgbaToPixmap.h"

namespace client_sim::ui {

QImage rgbaToQImage(const uint8_t* data, int width, int height) {
    if (data == nullptr || width <= 0 || height <= 0) {
        return QImage{};
    }
    // Construct a QImage that aliases the input buffer, then copy()
    // it so the returned image owns its pixels. This is the
    // documented way to get a deep-copy QImage from a raw buffer.
    QImage aliased(reinterpret_cast<const uchar*>(data),
                   width, height,
                   static_cast<qsizetype>(width) * 4,
                   QImage::Format_RGBA8888);
    return aliased.copy();
}

}  // namespace client_sim::ui
