// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
SPDX-FileCopyrightText: 2026 Matej Kocourek <matej@kocourek.cz>

SPDX-License-Identifier: GPL-2.0-or-later
*/
// Self
#include "cmsformats.h"

namespace Gwenview
{

namespace Cms
{

cmsUInt32Number pixelFormat(QImage::Format format)
{
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        return TYPE_BGRA_8;
    case QImage::Format_Grayscale8:
        return TYPE_GRAY_8;
    case QImage::Format_RGB888:
        return TYPE_RGB_8;
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        return TYPE_RGBA_8;
    case QImage::Format_Grayscale16:
        return TYPE_GRAY_16;
    case QImage::Format_RGBA64:
    case QImage::Format_RGBX64:
        return TYPE_RGBA_16;
    case QImage::Format_BGR888:
        return TYPE_BGR_8;
    default:
        return 0;
    }
}

QImage::Format transformableFormat(const QImage &image)
{
    QImage::Format format = image.format();

    if (image.colorCount() > 0) {
        format = image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
    } else {
        switch (format) {
        case QImage::Format_ARGB32_Premultiplied:
            format = QImage::Format_ARGB32;
            break;
        case QImage::Format_RGBA8888_Premultiplied:
            format = QImage::Format_RGBA8888;
            break;
        case QImage::Format_RGBA64_Premultiplied:
            format = QImage::Format_RGBA64;
            break;
        default:
            break;
        }
    }

    return pixelFormat(format) != 0 ? format : QImage::Format_Invalid;
}

} // namespace Cms
} // namespace Gwenview
