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

#ifndef RASTERIMAGEITEM_H
#define RASTERIMAGEITEM_H

#include <QGraphicsItem>

#include "lib/renderingintent.h"

namespace Gwenview
{
class RasterImageView;

/**
 * A QGraphicsItem subclass responsible for rendering the main raster image.
 *
 * This class is resposible for painting the main image when it is a raster
 * image. It will get the visible area from the main image, translate and scale
 * this based on the values from the parent ImageView, then apply color
 * correction. Finally the result will be drawn to the screen.
 *
 * For performance, two extra images are cached, one at a third of the image
 * size and one at a sixth. These are used at low zoom levels, to avoid having
 * to copy large amounts of image data that later gets discarded.
 */
class RasterImageItem : public QGraphicsItem
{
public:
    RasterImageItem(RasterImageView *parent);
    ~RasterImageItem() override;

    /**
     * Set the rendering intent for color correction.
     */
    void setRenderingIntent(RenderingIntent::Enum intent);

    /**
     * Update the internal, smaller cached versions of the main image.
     */
    void updateCache();

    /**
     * Reimplemented from QGraphicsItem::paint
     */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    /**
     * Reimplemented from QGraphicsItem::boundingRect
     */
    virtual QRectF boundingRect() const override;

private:
    void applyDisplayTransform(QImage &image);
    void updateDisplayTransform(QImage::Format format);

    RasterImageView *mParentView;
    bool mApplyDisplayTransform = true;
    cmsHTRANSFORM mDisplayTransform = nullptr;
    cmsUInt32Number mRenderingIntent = INTENT_PERCEPTUAL;

    QImage mThirdScaledImage;
    QImage mSixthScaledImage;
};

}

#endif // RASTERIMAGEITEM_H
