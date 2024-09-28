/*
Gwenview: an image viewer
Copyright 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/

#include "rasterimageitem.h"

#include <cmath>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

#include "gvdebug.h"
#include "lib/cms/cmsprofile.h"
#include "rasterimageview.h"

using namespace Gwenview;

// Convenience constants for one third and one sixth.
static const qreal Third = 1.0 / 3.0;
static const qreal Sixth = 1.0 / 6.0;

RasterImageItem::RasterImageItem(Gwenview::RasterImageView *parent)
    : QGraphicsItem(parent)
    , mParentView(parent)
{
}

RasterImageItem::~RasterImageItem()
{
    if (mDisplayTransform) {
        cmsDeleteTransform(mDisplayTransform);
    }
}

void RasterImageItem::setRenderingIntent(RenderingIntent::Enum intent)
{
    mRenderingIntent = intent;
    update();
}

void Gwenview::RasterImageItem::updateCache()
{
    auto document = mParentView->document();

    // Save a shallow copy of the image to make sure that it will not get
    // destroyed by another thread.
    mOriginalImage = document->image();

    // Cache two scaled down versions of the image, one at a third of the size
    // and one at a sixth. These are used instead of the document image at small
    // zoom levels, to avoid having to copy around the entire image which can be
    // very slow for large images.
    mThirdScaledImage = mOriginalImage.scaled(document->size() * Third, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    mSixthScaledImage = mOriginalImage.scaled(document->size() * Sixth, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void RasterImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    if (mOriginalImage.isNull() || mThirdScaledImage.isNull() || mSixthScaledImage.isNull()) {
        return;
    }

    const auto dpr = mParentView->devicePixelRatio();
    const auto zoom = mParentView->zoom();

    // This assumes we always have at least a single view of the graphics scene,
    // which should be true when painting a graphics item.
    const auto viewportRect = mParentView->scene()->views().first()->rect();

    // Map the viewport to the image so we get the area of the image that is
    // visible.
    auto imageRect = mParentView->mapToImage(viewportRect);

    // Grow the resulting rect by an arbitrary but small amount to avoid pixel
    // alignment issues. This results in the image being drawn slightly larger
    // than the viewport.
    imageRect = imageRect.marginsAdded(QMargins(5 * dpr, 5 * dpr, 5 * dpr, 5 * dpr));

    // Constrain the visible area rect by the image's rect so we don't try to
    // copy pixels that are outside the image.
    imageRect = imageRect.intersected(mOriginalImage.rect());

    QImage image;
    qreal targetZoom = zoom;

    // Copy the visible area from the document's image into a new image. This
    // allows us to modify the resulting image without affecting the original
    // image data. If we are zoomed out far enough, we instead use one of the
    // cached scaled copies to avoid having to copy a lot of data.
    if (zoom > Third) {
        image = mOriginalImage.copy(imageRect);
    } else if (zoom > Sixth) {
        auto sourceRect = QRect{imageRect.topLeft() * Third, imageRect.size() * Third};
        targetZoom = zoom / Third;
        image = mThirdScaledImage.copy(sourceRect);
    } else {
        auto sourceRect = QRect{imageRect.topLeft() * Sixth, imageRect.size() * Sixth};
        targetZoom = zoom / Sixth;
        image = mSixthScaledImage.copy(sourceRect);
    }

    const QImage::Format originalImageFormat = image.format();
    const bool isIndexedColor = !image.colorTable().isEmpty();
    const bool hasAlphaChannel = image.hasAlphaChannel();

    // We want nearest neighbour at high zoom since that provides the most
    // accurate representation of pixels, but at low zoom or when zooming out it
    // will not look very nice, so use smoothing instead. Switch at an arbitrary
    // threshold of 400% zoom
    const auto transformationMode = zoom < 4.0 ? Qt::SmoothTransformation : Qt::FastTransformation;

    // Scale the visible image to the requested zoom.
    image = image.scaled(image.size() * targetZoom, Qt::IgnoreAspectRatio, transformationMode);

    // If the original format is indexed, convert to Format_(A)RGB32 (the default format for loading PNG files).
    // (ARGB32 is necessary for pngquant'd transparent images to show Gwenview's background color.)
    // Otherwise scaling may convert image to premultiplied formats (unsupported by color correction engine).
    // Pray originalImageFormat is not premultiplied, and convert image back to it.
    if (isIndexedColor) {
        image.convertTo(hasAlphaChannel ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    } else if (image.format() != originalImageFormat) {
        image.convertTo(originalImageFormat);
    }

    // Perform color correction on the visible image.
    applyDisplayTransform(image);

    const auto destinationRect = QRectF{QPointF{std::ceil(imageRect.left() * (zoom / dpr)), std::ceil(imageRect.top() * (zoom / dpr))},
                                        QSizeF{image.size().width() / dpr, image.size().height() / dpr}};

    painter->drawImage(destinationRect, image);
}

QRectF RasterImageItem::boundingRect() const
{
    return QRectF{QPointF{0, 0}, mParentView->documentSize() * mParentView->zoom()};
}

void RasterImageItem::applyDisplayTransform(QImage &image)
{
    if (mApplyDisplayTransform) {
        updateDisplayTransform(image.format());
        if (mDisplayTransform) {
            quint8 *bytes = image.bits();
            cmsDoTransform(mDisplayTransform, bytes, bytes, image.width() * image.height());
        }
    }
}

void RasterImageItem::updateDisplayTransform(QImage::Format format)
{
    if (format == QImage::Format_Invalid) {
        return;
    }

    mApplyDisplayTransform = false;
    if (mDisplayTransform) {
        cmsDeleteTransform(mDisplayTransform);
    }
    mDisplayTransform = nullptr;

    Cms::Profile::Ptr profile = mParentView->document()->cmsProfile();
    if (!profile) {
        // The assumption that something unmarked is *probably* sRGB is better than failing to apply any transform when one
        // has a wide-gamut screen.
        profile = Cms::Profile::getSRgbProfile();
    }
    Cms::Profile::Ptr monitorProfile = Cms::Profile::getMonitorProfile();
    if (!monitorProfile) {
        qCWarning(GWENVIEW_LIB_LOG) << "Could not get monitor color profile";
        return;
    }

    cmsUInt32Number cmsFormat = 0;
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        cmsFormat = TYPE_BGRA_8;
        break;
    case QImage::Format_Grayscale8:
        cmsFormat = TYPE_GRAY_8;
        break;
    case QImage::Format_RGB888:
        cmsFormat = TYPE_RGB_8;
        break;
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        cmsFormat = TYPE_RGBA_8;
        break;
    case QImage::Format_Grayscale16:
        cmsFormat = TYPE_GRAY_16;
        break;
    case QImage::Format_RGBA64:
    case QImage::Format_RGBX64:
        cmsFormat = TYPE_RGBA_16;
        break;
    case QImage::Format_BGR888:
        cmsFormat = TYPE_BGR_8;
        break;
    default:
        qCWarning(GWENVIEW_LIB_LOG) << "Gwenview cannot apply color profile on" << format << "images";
        return;
    }

    mDisplayTransform =
        cmsCreateTransform(profile->handle(), cmsFormat, monitorProfile->handle(), cmsFormat, mRenderingIntent, cmsFLAGS_BLACKPOINTCOMPENSATION);
    mApplyDisplayTransform = true;
}
