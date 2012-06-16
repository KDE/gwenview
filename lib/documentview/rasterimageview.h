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

// KDE

class QGraphicsSceneHoverEvent;

namespace Gwenview
{

class AbstractRasterImageViewTool;

class RasterImageViewPrivate;
class GWENVIEWLIB_EXPORT RasterImageView : public AbstractImageView
{
    Q_OBJECT
public:
    enum AlphaBackgroundMode {
        AlphaBackgroundCheckBoard,
        AlphaBackgroundSolid
    };

    RasterImageView(QGraphicsItem* parent = 0);
    ~RasterImageView();

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    void setCurrentTool(AbstractRasterImageViewTool* tool);
    AbstractRasterImageViewTool* currentTool() const;

    void setAlphaBackgroundMode(AlphaBackgroundMode mode);
    void setAlphaBackgroundColor(const QColor& color);

Q_SIGNALS:
    void currentToolChanged(AbstractRasterImageViewTool*);

protected:
    void loadFromDocument();
    void onZoomChanged();
    void onImageOffsetChanged();
    void onScrollPosChanged(const QPointF& oldPos);
    void resizeEvent(QGraphicsSceneResizeEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    void wheelEvent(QGraphicsSceneWheelEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);

private Q_SLOTS:
    void slotDocumentMetaInfoLoaded();
    void slotDocumentIsAnimatedUpdated();
    void finishSetDocument();
    void updateFromScaler(int, int, const QImage&);
    void updateImageRect(const QRect& imageRect);
    void updateBuffer(const QRegion& region = QRegion());

private:
    bool paintGL(QPainter *painter);
    RasterImageViewPrivate* const d;
};

} // namespace

#endif /* RASTERIMAGEVIEW_H */
