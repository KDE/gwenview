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
#include "graphicshudslider.moc"

// Local
#include <fullscreentheme.h>

// KDE
#include <KDebug>
#include <KGlobalSettings>
#include <KIconLoader>

// Qt
#include <QApplication>
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QAbstractSlider>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

namespace Gwenview
{

struct GraphicsHudSliderPrivate
{
    GraphicsHudSlider* q;
    int mMin, mMax, mPageStep, mSingleStep;
    int mSliderPosition;
    int mValue;
    bool mIsDown;

    QRectF mHandleRect;

    void updateHandleRect()
    {
        static const FullScreenTheme::RenderInfo renderInfo = FullScreenTheme::renderInfo(FullScreenTheme::SliderWidgetHandle);
        static const int radius = renderInfo.borderRadius;

        const QRectF sliderRect = q->boundingRect();
        const qreal posX = xForPosition(mSliderPosition) - radius;
        const qreal posY = sliderRect.height() / 2 - radius;
        mHandleRect = QRectF(posX, posY, radius * 2, radius * 2);
    }

    int positionForX(qreal x) const
    {
        static const FullScreenTheme::RenderInfo renderInfo = FullScreenTheme::renderInfo(FullScreenTheme::SliderWidgetHandle);
        static const int radius = renderInfo.borderRadius;

        const qreal sliderWidth = q->boundingRect().width();

        x -= radius;
        if (QApplication::isRightToLeft()) {
            x = sliderWidth - 2 * radius - x;
        }
        return mMin + int(x / (sliderWidth - 2 * radius) * (mMax - mMin));
    }

    qreal xForPosition(int pos) const
    {
        static const FullScreenTheme::RenderInfo renderInfo = FullScreenTheme::renderInfo(FullScreenTheme::SliderWidgetHandle);
        static const int radius = renderInfo.borderRadius;

        const qreal sliderWidth = q->boundingRect().width();
        qreal x = (qreal(pos - mMin) / (mMax - mMin)) * (sliderWidth - 2 * radius);
        if (QApplication::isRightToLeft()) {
            x = sliderWidth - 2 * radius - x;
        }
        return x + radius;
    }
};

GraphicsHudSlider::GraphicsHudSlider(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new GraphicsHudSliderPrivate)
{
    d->q = this;
    d->mMin = 0;
    d->mMax = 100;
    d->mPageStep = 10;
    d->mSingleStep = 1;
    d->mSliderPosition = d->mValue = 0;
    d->mIsDown = false;
    setCursor(Qt::ArrowCursor);
    setAcceptHoverEvents(true);
}

GraphicsHudSlider::~GraphicsHudSlider()
{
    delete d;
}

void GraphicsHudSlider::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    FullScreenTheme::State state;
    if (option->state.testFlag(QStyle::State_MouseOver)) {
        state = d->mIsDown ? FullScreenTheme::DownState : FullScreenTheme::MouseOverState;
    } else {
        state = FullScreenTheme::NormalState;
    }
    painter->setRenderHint(QPainter::Antialiasing);

    const QRectF sliderRect = boundingRect();

    // Groove
    FullScreenTheme::RenderInfo renderInfo = FullScreenTheme::renderInfo(FullScreenTheme::SliderWidgetGroove, state);
    painter->setPen(renderInfo.borderPen);
    painter->setBrush(renderInfo.bgBrush);
    qreal centerY = d->mHandleRect.center().y();
    QRectF grooveRect = QRectF(
        0, centerY - renderInfo.borderRadius,
        sliderRect.width(),
        2 * renderInfo.borderRadius
        );

    // Clip out handle
    QPainterPath clipPath;
    clipPath.addRect(QRectF(QPointF(0, 0), d->mHandleRect.bottomLeft()).adjusted(0, 0, 1, 0));
    clipPath.addRect(QRectF(d->mHandleRect.topRight(), sliderRect.bottomRight()).adjusted(-1, 0, 0, 0));
    painter->setClipPath(clipPath);
    painter->drawRoundedRect(grooveRect.adjusted(.5, .5, -.5, -.5), renderInfo.borderRadius, renderInfo.borderRadius);
    painter->setClipping(false);

    // Handle
    renderInfo = FullScreenTheme::renderInfo(FullScreenTheme::SliderWidgetHandle, state);
    painter->setPen(renderInfo.borderPen);
    painter->setBrush(renderInfo.bgBrush);
    painter->drawRoundedRect(d->mHandleRect.adjusted(.5, .5, -.5, -.5), renderInfo.borderRadius, renderInfo.borderRadius);
}

void GraphicsHudSlider::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (boundingRect().contains(event->pos())) {
        switch (event->button()) {
        case Qt::LeftButton:
            d->mIsDown = true;
            break;
        case Qt::MiddleButton:
            setSliderPosition(d->positionForX(event->pos().x()));
            actionTriggered(QAbstractSlider::SliderMove);
            setValue(d->mSliderPosition);
            break;
        default:
            break;
        }
    }
    update();
}

void GraphicsHudSlider::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mIsDown) {
        setSliderPosition(d->positionForX(event->pos().x()));
        actionTriggered(QAbstractSlider::SliderMove);
        setValue(d->mSliderPosition);
        update();
    }
}

void GraphicsHudSlider::mouseReleaseEvent(QGraphicsSceneMouseEvent* /*event*/)
{
    d->mIsDown = false;
    update();
    /*
    if (boundingRect().contains(event->pos())) {
        clicked();
    }
    */
}

void GraphicsHudSlider::setRange(int min, int max)
{
    if (min == d->mMin && max == d->mMax) {
        return;
    }
    d->mMin = min;
    d->mMax = max;
    setValue(d->mValue); // ensure value is within min and max
    d->updateHandleRect();
    update();
}

void GraphicsHudSlider::setPageStep(int step)
{
    d->mPageStep = step;
}

void GraphicsHudSlider::setSingleStep(int step)
{
    d->mSingleStep = step;
}

void GraphicsHudSlider::setValue(int value)
{
    value = qBound(d->mMin, value, d->mMax);
    if (value != d->mValue) {
        d->mValue = value;
        setSliderPosition(value);
        update();
        valueChanged(d->mValue);
    }
}

int GraphicsHudSlider::sliderPosition() const
{
    return d->mSliderPosition;
}

void GraphicsHudSlider::setSliderPosition(int pos)
{
    pos = qBound(d->mMin, pos, d->mMax);
    if (pos != d->mSliderPosition) {
        d->mSliderPosition = pos;
        d->updateHandleRect();
        update();
    }
}

bool GraphicsHudSlider::isSliderDown() const
{
    return d->mIsDown;
}

} // namespace
