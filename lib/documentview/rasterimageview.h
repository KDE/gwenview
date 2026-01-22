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

// KF

class QGraphicsSceneHoverEvent;

namespace Gwenview
{
class AbstractRasterImageViewTool;

struct RasterImageViewPrivate;
class GWENVIEWLIB_EXPORT RasterImageView : public AbstractImageView
{
    Q_OBJECT
public:
    explicit RasterImageView(QGraphicsItem *parent = nullptr);
    ~RasterImageView() override;

    void setCurrentTool(AbstractRasterImageViewTool *tool);
    AbstractRasterImageViewTool *currentTool() const;

Q_SIGNALS:
    void currentToolChanged(AbstractRasterImageViewTool *);
    void imageRectUpdated();

protected:
    void loadFromDocument() override;
    void onZoomChanged() override;
    void onImageOffsetChanged() override;
    void onScrollPosChanged(const QPointF &oldPos) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;

private Q_SLOTS:
    void slotDocumentMetaInfoLoaded();
    void slotDocumentIsAnimatedUpdated();
    void finishSetDocument();

private:
    RasterImageViewPrivate *const d;
};

} // namespace

#endif /* RASTERIMAGEVIEW_H */
