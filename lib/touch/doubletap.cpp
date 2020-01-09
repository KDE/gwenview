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
#include "doubletap.h"

// Qt
#include <QTouchEvent>
#include "gwenview_lib_debug.h"
#include <QVector2D>
#include <QGraphicsWidget>

// KDE

// Local
#include "lib/touch/touch_helper.h"


namespace Gwenview
{

struct DoubleTapRecognizerPrivate
{
    DoubleTapRecognizer* q;
    bool mTargetIsGrapicsWidget = false;
    qint64 mTouchBeginnTimestamp;
    bool mIsOnlyTap;
    qint64 mLastTapTimestamp = 0;
    qint64 mLastDoupleTapTimestamp = 0;
};

DoubleTapRecognizer::DoubleTapRecognizer() : QGestureRecognizer()
, d (new DoubleTapRecognizerPrivate)
{
    d->q = this;
}

DoubleTapRecognizer::~DoubleTapRecognizer()
{
    delete d;
}

QGesture* DoubleTapRecognizer::create(QObject*)
{
    return static_cast<QGesture*>(new DoubleTap());
}

QGestureRecognizer::Result DoubleTapRecognizer::recognize(QGesture* state, QObject* watched, QEvent* event)
{
    //Because of a bug in Qt in a gesture event in a graphicsview, all gestures are trigger twice
    //https://bugreports.qt.io/browse/QTBUG-13103
    if (qobject_cast<QGraphicsWidget*>(watched)) d->mTargetIsGrapicsWidget = true;
    if (d->mTargetIsGrapicsWidget && watched->isWidgetType()) return Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
        d->mTouchBeginnTimestamp = touchEvent->timestamp();
        d->mIsOnlyTap = true;
        if (d->mLastDoupleTapTimestamp == 0) d->mLastDoupleTapTimestamp = touchEvent->timestamp() - Touch_Helper::Touch::doubleTapInterval;
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());
        return MayBeGesture;
    }

    case QEvent::TouchUpdate: {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
        const qint64 now = touchEvent->timestamp();
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());

        if (d->mIsOnlyTap && now - d->mTouchBeginnTimestamp < Touch_Helper::Touch::maxTimeForTap && Touch_Helper::touchStationary(event)) {
            d->mIsOnlyTap = true;
            return MayBeGesture;
        } else {
            d->mIsOnlyTap = false;
            return CancelGesture;
        }
        break;
    }

    case QEvent::TouchEnd: {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
        const qint64 now = touchEvent->timestamp();

        if (now - d->mLastTapTimestamp <= Touch_Helper::Touch::doubleTapInterval && d->mIsOnlyTap) {
            //Interval between two double tap gesture need to be bigger than Touch_Helper::Touch::doupleTapIntervall,
            //to suppress fast successively double tap gestures
            if (now - d->mLastDoupleTapTimestamp > Touch_Helper::Touch::doubleTapInterval) {
                d->mLastTapTimestamp = 0;
                state->setHotSpot(touchEvent->touchPoints().first().screenPos());
                d->mLastDoupleTapTimestamp = now;
                return FinishGesture;
            }
        }

        if (d->mIsOnlyTap)  d->mLastTapTimestamp = now;

        break;
    }

    default:
        return Ignore;
    }
    return Ignore;
}

DoubleTap::DoubleTap(QObject* parent)
: QGesture(parent)
{
}

} // namespace
