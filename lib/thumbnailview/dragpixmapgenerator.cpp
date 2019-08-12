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
#include <paintutils.h>

// KDE
#include <KIconLoader>

// Qt
#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QtMath>
#include <QDebug>

namespace Gwenview
{

namespace DragPixmapGenerator
{

const int DRAG_THUMB_SIZE = KIconLoader::SizeHuge;
const int SPREAD_ANGLE = 30;

DragPixmap generate(const QList<QPixmap>& pixmaps, int totalCount)
{
    DragPixmap out;

    const int extraSpace = DRAG_THUMB_SIZE / 4;
    const int maxHeight = qSqrt(qPow(DRAG_THUMB_SIZE + extraSpace, 2) + qPow(DRAG_THUMB_SIZE / 2, 2));
    out.pix = QPixmap(maxHeight * 2, maxHeight);
    out.pix.fill(Qt::transparent);

    QPainter painter(&out.pix);
    painter.translate(out.pix.width() / 2, out.pix.height());

    const int count = pixmaps.count();
    qreal delta = 0;
    if (count > 1) {
        painter.rotate(-SPREAD_ANGLE / 2);
        delta = SPREAD_ANGLE / (count - 1);
    }

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    //int index = 0;
    int maxX = 0;
    for (const QPixmap& pix : pixmaps) {
        QPixmap pix2 = pix.scaled(DRAG_THUMB_SIZE - 2, DRAG_THUMB_SIZE - 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect rect(-pix2.width() / 2, -pix2.height() - extraSpace, pix2.width(), pix2.height());

        if (!pix2.hasAlphaChannel()) {
            // Draw a thin white border around fully opaque pictures to give them a photo-like appearance
            painter.fillRect(rect.adjusted(-1, -1, 1, 1), Qt::white);
        }
        painter.drawPixmap(rect.topLeft(), pix2);

        QPoint topRight = painter.transform().map(rect.topRight());
        maxX = qMax(topRight.x(), maxX);
        /*
        painter.drawRect(-pix2.width() / 2, -pix2.height() - extraSpace, pix2.width(), pix2.height());
        painter.drawText(-pix2.width() / 2, -pix2.height() - extraSpace, pix2.width(), pix2.height(), Qt::AlignTop | Qt::AlignLeft, QString::number(index));
        index++;
        */
        painter.rotate(delta);
    }
    out.hotSpot = QPoint(out.pix.width() / 2, -extraSpace);

    if (count < totalCount) {
        painter.resetTransform();
        QString text = QString::number(totalCount);
        QRect rect = painter.fontMetrics().boundingRect(text);
        if (rect.width() < rect.height()) {
            rect.setWidth(rect.height());
        }

        const int padding = 1;
        rect.moveRight(maxX - padding - 1);
        rect.moveBottom(out.pix.height() - padding - 1);

        // Background
        QColor bg1 = QApplication::palette().color(QPalette::Highlight);
        QColor bg2 = PaintUtils::adjustedHsv(bg1, 0, 0, -50);
        QLinearGradient gradient(0, rect.top(), 0, rect.bottom());
        gradient.setColorAt(0, bg1);
        gradient.setColorAt(1, bg2);
        painter.setBrush(gradient);
        painter.setPen(bg1);
        painter.translate(-.5, -.5);
        painter.drawRoundedRect(rect.adjusted(-padding, -padding, padding, padding), padding, padding);

        // Foreground
        painter.setPen(QApplication::palette().color(QPalette::HighlightedText));
        painter.drawText(rect, Qt::AlignCenter, text);
    }

    return out;
}

} // DragPixmapGenerator namespace

} // namespace
