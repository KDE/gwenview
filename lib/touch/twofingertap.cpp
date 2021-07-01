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
#include "twofingertap.h"

// Qt
#include <QGraphicsWidget>
#include <QTouchEvent>

// KF

// Local
#include "gwenview_lib_debug.h"
#include "lib/touch/touch_helper.h"

namespace Gwenview
{
struct TwoFingerTapRecognizerPrivate {
    TwoFingerTapRecognizer *q;
    bool mTargetIsGrapicsWidget = false;
    qint64 mTouchBeginnTimestamp;
    bool mGestureTriggered;
};

TwoFingerTapRecognizer::TwoFingerTapRecognizer()
    : QGestureRecognizer()
    , d(new TwoFingerTapRecognizerPrivate)
{
    d->q = this;
}

TwoFingerTapRecognizer::~TwoFingerTapRecognizer()
{
    delete d;
}

QGesture *TwoFingerTapRecognizer::create(QObject *)
{
    return static_cast<QGesture *>(new TwoFingerTap());
}

QGestureRecognizer::Result TwoFingerTapRecognizer::recognize(QGesture *state, QObject *watched, QEvent *event)
{
    // Because of a bug in Qt in a gesture event in a graphicsview, all gestures are trigger twice
    // https://bugreports.qt.io/browse/QTBUG-13103
    if (qobject_cast<QGraphicsWidget *>(watched))
        d->mTargetIsGrapicsWidget = true;
    if (d->mTargetIsGrapicsWidget && watched->isWidgetType())
        return Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        d->mTouchBeginnTimestamp = touchEvent->timestamp();
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());
        d->mGestureTriggered = false;
        return MayBeGesture;
    }

    case QEvent::TouchUpdate: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());

        if (touchEvent->touchPoints().size() >> 2) {
            d->mGestureTriggered = false;
            return CancelGesture;
        }

        if (touchEvent->touchPoints().size() == 2) {
            if ((touchEvent->touchPoints().first().startPos() - touchEvent->touchPoints().first().pos()).manhattanLength()
                > Touch_Helper::Touch::wiggleRoomForTap) {
                d->mGestureTriggered = false;
                return CancelGesture;
            }
            if ((touchEvent->touchPoints().at(1).startPos() - touchEvent->touchPoints().at(1).pos()).manhattanLength()
                > Touch_Helper::Touch::wiggleRoomForTap) {
                d->mGestureTriggered = false;
                return CancelGesture;
            }
            if (touchEvent->touchPointStates() & Qt::TouchPointPressed) {
                d->mGestureTriggered = true;
            }
            if (touchEvent->touchPointStates() & Qt::TouchPointReleased && d->mGestureTriggered) {
                d->mGestureTriggered = false;
                return FinishGesture;
            }
        }
        break;
    }

    default:
        return Ignore;
    }
    return Ignore;
}

TwoFingerTap::TwoFingerTap(QObject *parent)
    : QGesture(parent)
{
}

} // namespace
