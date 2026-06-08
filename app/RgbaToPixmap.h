#pragma once

/**
 * @file RgbaToPixmap.h
 * @brief Convert WzPng::decodeRgba() output into a QImage.
 *
 * The byte order produced by wzlib (R, G, B, A; 8 bits per channel;
 * top-to-bottom; no padding) matches QImage::Format_RGBA8888 exactly,
 * so this is a wrap-and-deep-copy. The returned QImage owns its
 * pixel buffer and is safe to keep after the source vector goes
 * out of scope.
 */

#include <cstdint>

#include <QImage>

namespace client_sim::ui {

/**
 * @brief Build a Format_RGBA8888 QImage from a wzlib RGBA8 buffer.
 *
 * @param data  Pointer to the first byte of the buffer. Must point
 *              to at least @c 4 * width * height bytes.
 * @param width Image width in pixels. Must be > 0.
 * @param height Image height in pixels. Must be > 0.
 * @return A QImage that owns a copy of @p data. Returns a null QImage
 *         if any precondition is violated.
 */
QImage rgbaToQImage(const uint8_t* data, int width, int height);

}  // namespace client_sim::ui
