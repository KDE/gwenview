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
// Self
#include "abstractrasterimageviewtool.h"

// Qt
#include <QGraphicsSceneWheelEvent>

// KF

// Local
#include "rasterimageview.h"

namespace Gwenview
{

struct AbstractRasterImageViewToolPrivate
{
    RasterImageView* mRasterImageView;
};

AbstractRasterImageViewTool::AbstractRasterImageViewTool(RasterImageView* view)
: QObject(view)
, d(new AbstractRasterImageViewToolPrivate)
{
    d->mRasterImageView = view;
}

AbstractRasterImageViewTool::~AbstractRasterImageViewTool()
{
    delete d;
}

RasterImageView* AbstractRasterImageViewTool::imageView() const
{
    return d->mRasterImageView;
}

void AbstractRasterImageViewTool::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    event->ignore();
}

void AbstractRasterImageViewTool::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    event->ignore();
}

} // namespace
