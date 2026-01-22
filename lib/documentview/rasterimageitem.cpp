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

RasterImageItem::~RasterImageItem() = default;

void Gwenview::RasterImageItem::updateCache()
{
    auto document = mParentView->document();

    // Save a shallow copy of the image to make sure that it will not get
    // destroyed by another thread.
    mOriginalImage = document->colorCorrectedImage();
}

void RasterImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    if (mOriginalImage.isNull()) {
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
    QRectF sourceRect;
    // Copy the visible area from the document's image into a new image. This
    // allows us to modify the resulting image without affecting the original
    // image data. If we are zoomed out far enough, we instead use one of the
    // cached scaled copies to avoid having to copy a lot of data.
    image = mOriginalImage;
    sourceRect = imageRect;

    // We want nearest neighbour at high zoom since that provides the most
    // accurate representation of pixels, but at low zoom or when zooming out it
    // will not look very nice, so use smoothing instead. Switch at an arbitrary
    // threshold of 400% zoom
    const auto useSmoothTransform = zoom < 4.0;

    const QRectF destinationRect(QPointF(std::ceil(imageRect.left() * (zoom / dpr)), std::ceil(imageRect.top() * (zoom / dpr))),
                                 QSizeF(sourceRect.width() * zoom / dpr, sourceRect.height() * zoom / dpr));

    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform, useSmoothTransform);
    painter->drawImage(destinationRect, image, sourceRect);
    painter->restore();
}

QRectF RasterImageItem::boundingRect() const
{
    return QRectF{QPointF{0, 0}, mParentView->documentSize() * mParentView->zoom()};
}
