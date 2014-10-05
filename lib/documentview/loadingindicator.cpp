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
#include "loadingindicator.h"

// Local

// KDE
#include <QDebug>
#include <KPixmapSequence>

// Qt
#include <QPainter>
#include <QTimer>

namespace Gwenview
{

struct LoadingIndicatorPrivate
{
    LoadingIndicator* q;
    KPixmapSequence mSequence;
    int mIndex;
    QTimer* mTimer;

    LoadingIndicatorPrivate(LoadingIndicator* qq)
    : q(qq)
    , mSequence("process-working", 22)
    , mIndex(0)
    , mTimer(new QTimer(qq))
    {
        mTimer->setInterval(100);
        QObject::connect(mTimer, SIGNAL(timeout()), q, SLOT(showNextFrame()));
    }
};

LoadingIndicator::LoadingIndicator(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new LoadingIndicatorPrivate(this))
{
}

LoadingIndicator::~LoadingIndicator()
{
    delete d;
}

QRectF LoadingIndicator::boundingRect() const
{
    return QRectF(QPointF(0, 0), d->mSequence.frameSize());
}

void LoadingIndicator::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->drawPixmap(0, 0, d->mSequence.frameAt(d->mIndex));
}

void LoadingIndicator::showNextFrame()
{
    if (d->mSequence.frameCount() > 0) {
        d->mIndex = (d->mIndex + 1) % d->mSequence.frameCount();
        update();
    }
}

QVariant LoadingIndicator::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemVisibleHasChanged) {
        if (value.toBool()) {
            d->mTimer->start();
        } else {
            d->mTimer->stop();
        }
    }
    return QGraphicsWidget::itemChange(change, value);
}

} // namespace
