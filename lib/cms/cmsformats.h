// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
SPDX-FileCopyrightText: 2026 Matej Kocourek <matej@kocourek.cz>

SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef CMSFORMATS_H
#define CMSFORMATS_H

// Qt
#include <QImage>

// lcms
#include <lcms2.h>

namespace Gwenview
{

namespace Cms
{

/**
 * The LittleCMS pixel format for @p format, or 0 if there is none.
 */
cmsUInt32Number pixelFormat(QImage::Format format);

/**
 * The format @p image must be converted to before a LittleCMS transform can be
 * applied, or QImage::Format_Invalid if LittleCMS cannot handle it at all.
 */
QImage::Format transformableFormat(const QImage &image);

} // namespace Cms
} // namespace Gwenview

#endif /* CMSFORMATS_H */
