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
// Self
#include <thumbnailview/dragpixmapgenerator.h>

// Local

// KDE
#include <KDebug>
#include <KIconLoader>
#include <KIcon>

// Qt
#include <QPainter>
#include <QPixmap>
#include <qmath.h>

namespace Gwenview
{

namespace DragPixmapGenerator
{

const int DRAG_THUMB_SIZE = KIconLoader::SizeHuge;
const int DRAG_THUMB_SPACING = 4;
const int SPREAD_ANGLE = 30;

DragPixmap generate(const QList<QPixmap>& pixmaps, bool more)
{
    DragPixmap out;

    const int extraSpace = DRAG_THUMB_SIZE / 4;
    const int maxHeight = qSqrt(qPow(DRAG_THUMB_SIZE + extraSpace, 2) + qPow(DRAG_THUMB_SIZE / 2, 2));
    out.pix = QPixmap(maxHeight * 2, maxHeight);
    out.pix.fill(Qt::transparent);

    QPainter painter(&out.pix);
    painter.translate(out.pix.width() / 2, out.pix.height());

    const int count = pixmaps.count();
    qreal delta;
    if (count > 1) {
        painter.rotate(-SPREAD_ANGLE / 2);
        delta = SPREAD_ANGLE / (count - 1);
    }

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    //int index = 0;
    int minX = out.pix.width();
    Q_FOREACH(const QPixmap& pix, pixmaps) {
        QPixmap pix2 = pix.scaled(DRAG_THUMB_SIZE - 2, DRAG_THUMB_SIZE - 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect rect(-pix2.width() / 2, -pix2.height() - extraSpace, pix2.width(), pix2.height());

        painter.fillRect(rect.adjusted(-1, -1, 1, 1), Qt::white);
        painter.drawPixmap(rect.topLeft(), pix2);

        QPoint topLeft = painter.transform().map(rect.topLeft());
        minX = qMin(topLeft.x(), minX);
        /*
        painter.drawRect(-pix2.width() / 2, -pix2.height() - extraSpace, pix2.width(), pix2.height());
        painter.drawText(-pix2.width() / 2, -pix2.height() - extraSpace, pix2.width(), pix2.height(), Qt::AlignTop | Qt::AlignLeft, QString::number(index));
        index++;
        */
        painter.rotate(delta);
    }
    kWarning() << "minX" << minX;
    out.hotSpot = QPoint(out.pix.width() / 2, -extraSpace);//out.pix.height());

    if (more) {
        painter.resetTransform();
        //painter.drawText(out.rect(), Qt::AlignRight | Qt::AlignBottom, "+");
        KIcon icon("list-add");
        QPixmap iconPix = icon.pixmap(KIconLoader::SizeSmallMedium);
        painter.drawPixmap(out.pix.width() - iconPix.width(), out.pix.height() - iconPix.height(), iconPix);
    }

    return out;
}

} // DragPixmapGenerator namespace

} // namespace
