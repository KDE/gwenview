/*
Gwenview: an image viewer
Copyright 2019 Steffen Hartleib <steffenhartleib@t-online.de>

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
#include "oneandtwofingerswipe.h"

// Qt
#include <QTouchEvent>
#include <QDebug>
#include <QGraphicsWidget>

// KDE

// Local
#include "lib/touch/touch_helper.h"

#include <math.h>

namespace Gwenview
{

struct OneAndTwoFingerSwipeRecognizerPrivate
{
    OneAndTwoFingerSwipeRecognizer* q;
    bool mTargetIsGrapicsWidget = false;
    qint64 mTouchBeginnTimestamp;
    bool mGestureAlreadyTriggered;
};

OneAndTwoFingerSwipeRecognizer::OneAndTwoFingerSwipeRecognizer() : QGestureRecognizer()
, d (new OneAndTwoFingerSwipeRecognizerPrivate)
{
    d->q = this;
}

OneAndTwoFingerSwipeRecognizer::~OneAndTwoFingerSwipeRecognizer()
{
    delete d;
}

QGesture* OneAndTwoFingerSwipeRecognizer::create(QObject*)
{
    return static_cast<QGesture*>(new OneAndTwoFingerSwipe());
}

QGestureRecognizer::Result OneAndTwoFingerSwipeRecognizer::recognize(QGesture* state, QObject* watched, QEvent* event)
{
    //Because of a bug in Qt in a gesture event in a graphicsview, all gestures are trigger twice
    //https://bugreports.qt.io/browse/QTBUG-13103
    if (qobject_cast<QGraphicsWidget*>(watched)) d->mTargetIsGrapicsWidget = true;
    if (d->mTargetIsGrapicsWidget && watched->isWidgetType()) return Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
        d->mTouchBeginnTimestamp = touchEvent->timestamp();
        d->mGestureAlreadyTriggered = false;
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());
        return MayBeGesture;
    }

    case QEvent::TouchUpdate: {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
        const qint64 now = touchEvent->timestamp();
        const QPointF distance = touchEvent->touchPoints().first().startPos() - touchEvent->touchPoints().first().pos();
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());

        if (touchEvent->touchPoints().size() >> 2) {
            d->mGestureAlreadyTriggered = false;
            return CancelGesture;
        }

        if (distance.manhattanLength() >= Touch_Helper::Touch::minDistanceForSwipe &&
                (now - d->mTouchBeginnTimestamp) <= Touch_Helper::Touch::maxTimeFrameForSwipe &&
                !d->mGestureAlreadyTriggered) {
            if (distance.x() < 0 && abs(distance.x()) >= abs(distance.y()) * 2) {
                state->setProperty("right", true);
                state->setProperty("left", false);
                d->mGestureAlreadyTriggered = true;
                return FinishGesture;
            }
            if (distance.x() > 0 && abs(distance.x()) >= abs(distance.y()) * 2) {
                state->setProperty("right", false);
                state->setProperty("left", true);
                d->mGestureAlreadyTriggered = true;
                return FinishGesture;
            }
            if ((now - d->mTouchBeginnTimestamp) <= Touch_Helper::Touch::maxTimeFrameForSwipe && !d->mGestureAlreadyTriggered) {
                return MayBeGesture;
            } else {
                d->mGestureAlreadyTriggered = false;
                return CancelGesture;
            }
        }
        break;
    }

    default:
        return Ignore;
    }
    return Ignore;
}

OneAndTwoFingerSwipe::OneAndTwoFingerSwipe(QObject* parent)
: QGesture(parent)
{
}

} // namespace
