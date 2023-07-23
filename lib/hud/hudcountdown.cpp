// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2014 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
// Self
#include "hudcountdown.h"

// Local
#include <lib/hud/hudtheme.h>

// KF

// Qt
#include <QPainter>
#include <QTimeLine>

namespace Gwenview
{
struct HudCountDownPrivate {
    QTimeLine *mTimeLine = nullptr;
};

HudCountDown::HudCountDown(QGraphicsWidget *parent)
    : QGraphicsWidget(parent)
    , d(new HudCountDownPrivate)
{
    d->mTimeLine = new QTimeLine(0, this);
    d->mTimeLine->setDirection(QTimeLine::Backward);
    connect(d->mTimeLine, &QTimeLine::valueChanged, this, &HudCountDown::doUpdate);
    connect(d->mTimeLine, &QTimeLine::finished, this, &HudCountDown::timeout);

    // Use an odd value so that the vertical line is aligned to pixel
    // boundaries
    setMinimumSize(17, 17);
}

HudCountDown::~HudCountDown()
{
    delete d;
}

void HudCountDown::start(qreal ms)
{
    d->mTimeLine->setDuration(ms);
    d->mTimeLine->start();
    update();
}

void HudCountDown::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    HudTheme::RenderInfo info = HudTheme::renderInfo(HudTheme::CountDown);
    painter->setRenderHint(QPainter::Antialiasing);
    const int circle = 5760;
    const int start = circle / 4; // Start at 12h, not 3h
    const int end = int(circle * d->mTimeLine->currentValue());
    painter->setBrush(info.bgBrush);
    painter->setPen(info.borderPen);

    QRectF square = boundingRect().adjusted(.5, .5, -.5, -.5);
    const qreal width = square.width();
    const qreal height = square.height();
    if (width < height) {
        square.setHeight(width);
        square.moveTop((height - width) / 2);
    } else {
        square.setWidth(height);
        square.moveLeft((width - height) / 2);
    }
    painter->drawPie(square, start, end);
}

void HudCountDown::doUpdate()
{
    update();
}

} // namespace

#include "moc_hudcountdown.cpp"
