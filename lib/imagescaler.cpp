/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "imagescaler.h"

// Qt
#include <QImage>
#include <QRegion>
#include <QApplication>

// KF

// Local
#include "gwenview_lib_debug.h"
#include <lib/document/document.h>
#include <lib/paintutils.h>

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qCDebug(GWENVIEW_LIB_LOG) << x
#else
#define LOG(x) ;
#endif

namespace Gwenview
{

// Amount of pixels to keep so that smooth scale is correct
static const int SMOOTH_MARGIN = 3;

static inline QRectF scaledRect(const QRectF& rect, qreal factor)
{
    return QRectF(rect.x() * factor,
                  rect.y() * factor,
                  rect.width() * factor,
                  rect.height() * factor);
}

static inline QRect scaledRect(const QRect& rect, qreal factor)
{
    return scaledRect(QRectF(rect), factor).toAlignedRect();
}

struct ImageScalerPrivate
{
    Qt::TransformationMode mTransformationMode;
    Document::Ptr mDocument;
    qreal mZoom;
    QRegion mRegion;
};

ImageScaler::ImageScaler(QObject* parent)
: QObject(parent)
, d(new ImageScalerPrivate)
{
    d->mTransformationMode = Qt::FastTransformation;
    d->mZoom = 0;
}

ImageScaler::~ImageScaler()
{
    delete d;
}

void ImageScaler::setDocument(const Document::Ptr &document)
{
    if (d->mDocument) {
        disconnect(d->mDocument.data(), nullptr, this, nullptr);
    }
    d->mDocument = document;
    // Used when scaler asked for a down-sampled image
    connect(d->mDocument.data(), &Document::downSampledImageReady,
            this, &ImageScaler::doScale);
    // Used when scaler asked for a full image
    connect(d->mDocument.data(), &Document::loaded,
            this, &ImageScaler::doScale);
}

void ImageScaler::setZoom(qreal zoom)
{
    // If we zoom to 400% or more, then assume the user wants to see the real
    // pixels, for example to fine tune a crop operation
    d->mTransformationMode = zoom < 4. ? Qt::SmoothTransformation
                                       : Qt::FastTransformation;

    d->mZoom = zoom;
}

void ImageScaler::setDestinationRegion(const QRegion& region)
{
    LOG(region);
    d->mRegion = region;
    if (d->mRegion.isEmpty()) {
        return;
    }

    if (d->mDocument && d->mZoom > 0) {
        doScale();
    }
}

void ImageScaler::doScale()
{
    if (d->mZoom < Document::maxDownSampledZoom()) {
        if (!d->mDocument->prepareDownSampledImageForZoom(d->mZoom)) {
            LOG("Asked for a down sampled image");
            return;
        }
    } else if (d->mDocument->image().isNull()) {
        LOG("Asked for the full image");
        d->mDocument->startLoadingFullImage();
        return;
    }

    LOG("Starting");
    for (const QRect &rect : qAsConst(d->mRegion)) {
        LOG(rect);
        scaleRect(rect);
    }
    LOG("Done");
}

void ImageScaler::scaleRect(const QRect& rect)
{
    const qreal dpr = qApp->devicePixelRatio();

    // variables prefixed with dp are in device pixels
    const QRect dpRect = Gwenview::scaledRect(rect, dpr);

    const qreal REAL_DELTA = 0.001;
    if (qAbs(d->mZoom - 1.0) < REAL_DELTA) {
        QImage tmp = d->mDocument->image().copy(dpRect);
        tmp.setDevicePixelRatio(dpr);
        emit scaledRect(rect.left(), rect.top(), tmp);
        return;
    }

    QImage image;
    qreal zoom;
    if (d->mZoom < Document::maxDownSampledZoom()) {
        image = d->mDocument->downSampledImageForZoom(d->mZoom);
        Q_ASSERT(!image.isNull());
        qreal zoom1 = qreal(image.width()) / d->mDocument->width();
        zoom = d->mZoom / zoom1;
    } else {
        image = d->mDocument->image();
        zoom = d->mZoom;
    }
    const QRect imageRect = Gwenview::scaledRect(image.rect(), 1.0 / dpr);

    // If rect contains "half" pixels, make sure sourceRect includes them
    QRectF sourceRectF = Gwenview::scaledRect(QRectF(rect), 1.0 / zoom);

    sourceRectF = sourceRectF.intersected(imageRect);
    QRect sourceRect = sourceRectF.toAlignedRect();
    if (sourceRect.isEmpty()) {
        return;
    }

    // Compute smooth margin
    bool needsSmoothMargins = d->mTransformationMode == Qt::SmoothTransformation;

    int sourceLeftMargin, sourceRightMargin, sourceTopMargin, sourceBottomMargin;
    int destLeftMargin, destRightMargin, destTopMargin, destBottomMargin;
    if (needsSmoothMargins) {
        sourceLeftMargin = qMin(sourceRect.left(), SMOOTH_MARGIN);
        sourceTopMargin = qMin(sourceRect.top(), SMOOTH_MARGIN);
        sourceRightMargin = qMin(imageRect.right() - sourceRect.right(), SMOOTH_MARGIN);
        sourceBottomMargin = qMin(imageRect.bottom() - sourceRect.bottom(), SMOOTH_MARGIN);
        sourceRect.adjust(
            -sourceLeftMargin,
            -sourceTopMargin,
            sourceRightMargin,
            sourceBottomMargin);
        destLeftMargin = int(sourceLeftMargin * zoom);
        destTopMargin = int(sourceTopMargin * zoom);
        destRightMargin = int(sourceRightMargin * zoom);
        destBottomMargin = int(sourceBottomMargin * zoom);
    } else {
        sourceLeftMargin = sourceRightMargin = sourceTopMargin = sourceBottomMargin = 0;
        destLeftMargin = destRightMargin = destTopMargin = destBottomMargin = 0;
    }

    // destRect is almost like rect, but it contains only "full" pixels
    QRect destRect = Gwenview::scaledRect(sourceRect, zoom);

    QRect dpSourceRect = Gwenview::scaledRect(sourceRect, dpr);
    QRect dpDestRect = Gwenview::scaledRect(dpSourceRect, zoom);

    QImage tmp;
    tmp = image.copy(dpSourceRect);
    tmp = tmp.scaled(
              dpDestRect.width(),
              dpDestRect.height(),
              Qt::IgnoreAspectRatio, // Do not use KeepAspectRatio, it can lead to skipped rows or columns
              d->mTransformationMode);

    if (needsSmoothMargins) {
        tmp = tmp.copy(
                  destLeftMargin * dpr, destTopMargin * dpr,
                  dpDestRect.width() - (destLeftMargin + destRightMargin) * dpr,
                  dpDestRect.height() - (destTopMargin + destBottomMargin) * dpr
              );
    }

    tmp.setDevicePixelRatio(dpr);
    emit scaledRect(destRect.left() + destLeftMargin, destRect.top() + destTopMargin, tmp);
}

} // namespace
