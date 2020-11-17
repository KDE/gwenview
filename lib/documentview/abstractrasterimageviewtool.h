// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef ABSTRACTRASTERIMAGEVIEWTOOL_H
#define ABSTRACTRASTERIMAGEVIEWTOOL_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>

// KF

// Local

class QKeyEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QPainter;

namespace Gwenview
{

class RasterImageView;

struct AbstractRasterImageViewToolPrivate;
class GWENVIEWLIB_EXPORT AbstractRasterImageViewTool : public QObject
{
    Q_OBJECT
public:
    AbstractRasterImageViewTool(RasterImageView* view);
    ~AbstractRasterImageViewTool() override;

    RasterImageView* imageView() const;

    virtual void paint(QPainter*)
    {}

    virtual void mousePressEvent(QGraphicsSceneMouseEvent*)
    {}
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*)
    {}
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*)
    {}
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent*)
    {}
    virtual void wheelEvent(QGraphicsSceneWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent*)
    {}
    virtual void keyReleaseEvent(QKeyEvent*)
    {}

    virtual void toolActivated()
    {}
    virtual void toolDeactivated()
    {}

    virtual QWidget* widget() const
    {
        return nullptr;
    }

public Q_SLOTS:
    virtual void onWidgetSlidedIn()
    {}

private:
    AbstractRasterImageViewToolPrivate * const d;
};

} // namespace

#endif /* ABSTRACTRASTERIMAGEVIEWTOOL_H */
