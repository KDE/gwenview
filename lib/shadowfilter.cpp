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
#include "shadowfilter.h"

// Local

// KF

// Qt
#include <QColor>
#include <QCoreApplication>
#include <QHash>
#include <QPainter>
#include <QWidget>

namespace Gwenview
{
struct ShadowFilterPrivate {
    QWidget *mWidget = nullptr;
    QHash<ShadowFilter::WidgetEdge, QColor> mShadows;

    void paintShadow(QPainter *painter, const QColor &color, QPoint origin, int dx, int dy)
    {
        const int gradientSize = 12;

        if (color == Qt::transparent) {
            return;
        }
        QLinearGradient gradient;
        gradient.setColorAt(0, color);
        gradient.setColorAt(1, Qt::transparent);
        gradient.setStart(origin);
        gradient.setFinalStop(origin.x() + dx * gradientSize, origin.y() + dy * gradientSize);
        painter->fillRect(mWidget->rect(), gradient);
    }

    void paint()
    {
        QPainter painter(mWidget);
        const QRect rect = mWidget->rect();
        QColor color;

        color = mShadows.value(ShadowFilter::LeftEdge, Qt::transparent);
        paintShadow(&painter, color, rect.topLeft(), 1, 0);

        color = mShadows.value(ShadowFilter::TopEdge, Qt::transparent);
        paintShadow(&painter, color, rect.topLeft(), 0, 1);

        color = mShadows.value(ShadowFilter::RightEdge, Qt::transparent);
        paintShadow(&painter, color, rect.topRight(), -1, 0);

        color = mShadows.value(ShadowFilter::BottomEdge, Qt::transparent);
        paintShadow(&painter, color, rect.bottomLeft(), 0, -1);
    }
};

ShadowFilter::ShadowFilter(QWidget *widget)
    : QObject(widget)
    , d(new ShadowFilterPrivate)
{
    d->mWidget = widget;
    widget->installEventFilter(this);
}

ShadowFilter::~ShadowFilter()
{
    delete d;
}

void ShadowFilter::setShadow(ShadowFilter::WidgetEdge edge, const QColor &color)
{
    d->mShadows[edge] = color;
}

void ShadowFilter::reset()
{
    d->mShadows.clear();
}

bool ShadowFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        obj->removeEventFilter(this);
        QCoreApplication::sendEvent(obj, event);
        d->paint();
        obj->installEventFilter(this);
        return true;
    }
    return false;
}

} // namespace
