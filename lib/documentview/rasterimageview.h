// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef RASTERIMAGEVIEW_H
#define RASTERIMAGEVIEW_H

#include <lib/gwenviewlib_export.h>

// Local
#include <lib/documentview/abstractimageview.h>
#include <lib/renderingintent.h>

// KDE

class QGraphicsSceneHoverEvent;

namespace Gwenview
{

class AbstractRasterImageViewTool;

struct RasterImageViewPrivate;
class GWENVIEWLIB_EXPORT RasterImageView : public AbstractImageView
{
    Q_OBJECT
public:
    RasterImageView(QGraphicsItem* parent = 0);
    ~RasterImageView();

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) Q_DECL_OVERRIDE;

    void setCurrentTool(AbstractRasterImageViewTool* tool);
    AbstractRasterImageViewTool* currentTool() const;

    void setAlphaBackgroundMode(AlphaBackgroundMode mode) override;
    void setAlphaBackgroundColor(const QColor& color) override;
    void setRenderingIntent(const RenderingIntent::Enum& renderingIntent);

Q_SIGNALS:
    void currentToolChanged(AbstractRasterImageViewTool*);
    void imageRectUpdated();

protected:
    void loadFromDocument() Q_DECL_OVERRIDE;
    void onZoomChanged() Q_DECL_OVERRIDE;
    void onImageOffsetChanged() Q_DECL_OVERRIDE;
    void onScrollPosChanged(const QPointF& oldPos) Q_DECL_OVERRIDE;
    void resizeEvent(QGraphicsSceneResizeEvent* event) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void wheelEvent(QGraphicsSceneWheelEvent* event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotDocumentMetaInfoLoaded();
    void slotDocumentIsAnimatedUpdated();
    void finishSetDocument();
    void updateFromScaler(int, int, const QImage&);
    void updateImageRect(const QRect& imageRect);
    void updateBuffer(const QRegion& region = QRegion());

private:
    RasterImageViewPrivate* const d;
};

} // namespace

#endif /* RASTERIMAGEVIEW_H */
