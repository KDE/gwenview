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
#include "tapholdandmoving.h"

// Qt
#include <QTouchEvent>
#include <QGraphicsWidget>

// KF

// Local
#include "gwenview_lib_debug.h"
#include "lib/touch/touch_helper.h"

namespace Gwenview
{

struct TapHoldAndMovingRecognizerPrivate
{
    TapHoldAndMovingRecognizer* q;
    bool mTargetIsGrapicsWidget = false;
    qint64 mTouchBeginnTimestamp;
    bool mTouchPointStationary;
    bool mGestureTriggered;
    Qt::GestureState mLastGestureState = Qt::NoGesture;
};

TapHoldAndMovingRecognizer::TapHoldAndMovingRecognizer() : QGestureRecognizer()
, d (new TapHoldAndMovingRecognizerPrivate)
{
    d->q = this;
}

TapHoldAndMovingRecognizer::~TapHoldAndMovingRecognizer()
{
    delete d;
}

QGesture* TapHoldAndMovingRecognizer::create(QObject*)
{
    return static_cast<QGesture*>(new TapHoldAndMoving());
}

QGestureRecognizer::Result TapHoldAndMovingRecognizer::recognize(QGesture* state, QObject* watched, QEvent* event)
{
    //Because of a bug in Qt in a gesture event in a graphicsview, all gestures are trigger twice
    //https://bugreports.qt.io/browse/QTBUG-13103
    if (qobject_cast<QGraphicsWidget*>(watched)) d->mTargetIsGrapicsWidget = true;
    if (d->mTargetIsGrapicsWidget && watched->isWidgetType()) return Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        auto* touchEvent = static_cast<QTouchEvent*>(event);
        d->mTouchBeginnTimestamp = touchEvent->timestamp();
        d->mGestureTriggered = false;
        d->mTouchPointStationary = true;
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());
        d->mLastGestureState = Qt::NoGesture;
        return MayBeGesture;
    }

    case QEvent::TouchUpdate: {
        auto* touchEvent = static_cast<QTouchEvent*>(event);
        const qint64 now = touchEvent->timestamp();
        const QPoint pos = touchEvent->touchPoints().first().pos().toPoint();
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());

        if (touchEvent->touchPoints().size() >> 1) {
            d->mGestureTriggered = false;
            d->mLastGestureState = Qt::GestureCanceled;
            return CancelGesture;
        }

        if (touchEvent->touchPoints().size() == 1 && d->mLastGestureState != Qt::GestureCanceled) {
            if (!d->mGestureTriggered &&
                        d->mTouchPointStationary && 
                        now - d->mTouchBeginnTimestamp >= Touch_Helper::Touch::durationForTapHold) {
                d->mGestureTriggered = true;
            }
        }
        d->mTouchPointStationary = Touch_Helper::touchStationary(event);

        if (d->mGestureTriggered && d->mLastGestureState != Qt::GestureCanceled) {
            state->setProperty("pos", pos);
            d->mLastGestureState = Qt::GestureStarted;
            return TriggerGesture;
        }
        break;
    }

    case QEvent::TouchEnd: {
        auto* touchEvent = static_cast<QTouchEvent*>(event);
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());
        if (d->mGestureTriggered) {
            d->mLastGestureState = Qt::GestureFinished;
            return FinishGesture;
        }
        break;
    }
    default:
        return Ignore;
    }
    return Ignore;
}

TapHoldAndMoving::TapHoldAndMoving(QObject* parent)
: QGesture(parent)
{
}

} // namespace
