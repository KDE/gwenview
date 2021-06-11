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
#include "twofingerpan.h"

// Qt
#include <QTouchEvent>
#include <QVector2D>
#include <QGraphicsWidget>

// KF

// Local
#include "gwenview_lib_debug.h"
#include "lib/touch/touch_helper.h"

namespace Gwenview
{

struct TwoFingerPanRecognizerPrivate
{
    TwoFingerPanRecognizer* q;
    bool mTargetIsGrapicsWidget = false;
    qint64 mTouchBeginnTimestamp;
    bool mGestureTriggered;
    qint64 mLastTouchTimestamp;
};

TwoFingerPanRecognizer::TwoFingerPanRecognizer() : QGestureRecognizer()
    , d (new TwoFingerPanRecognizerPrivate)
{
    d->q = this;
}

TwoFingerPanRecognizer::~TwoFingerPanRecognizer()
{
    delete d;
}

QGesture* TwoFingerPanRecognizer::create(QObject*)
{
    return static_cast<QGesture*>(new TwoFingerPan());
}

QGestureRecognizer::Result TwoFingerPanRecognizer::recognize(QGesture* state, QObject* watched, QEvent* event)
{
    //Because of a bug in Qt in a gesture event in a graphicsview, all gestures are trigger twice
    //https://bugreports.qt.io/browse/QTBUG-13103
    if (qobject_cast<QGraphicsWidget*>(watched)) d->mTargetIsGrapicsWidget = true;
    if (d->mTargetIsGrapicsWidget && watched->isWidgetType()) return Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        auto* touchEvent = static_cast<QTouchEvent*>(event);
        d->mTouchBeginnTimestamp = touchEvent->timestamp();
        d->mLastTouchTimestamp = touchEvent->timestamp();
        d->mGestureTriggered = false;
        state->setProperty("delayActive", true);
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());
        return TriggerGesture;
    }

    case QEvent::TouchUpdate: {
        auto* touchEvent = static_cast<QTouchEvent*>(event);
        const qint64 now = touchEvent->timestamp();
        const QPointF pos = touchEvent->touchPoints().first().pos();
        state->setHotSpot(touchEvent->touchPoints().first().screenPos());

        if (touchEvent->touchPoints().size() >> 2) {
            if (d->mGestureTriggered) {
                d->mGestureTriggered = false;
            }
            return CancelGesture;
        }

        if (touchEvent->touchPoints().size() == 2) {
            if (touchEvent->touchPointStates() & Qt::TouchPointReleased) {
                if (d->mGestureTriggered) {
                    d->mGestureTriggered = false;
                    return FinishGesture;
                }
            }

            if (now - d->mTouchBeginnTimestamp >= Touch_Helper::Touch::gestureDelay) {
                state ->setProperty("delayActive", false);

                //Check if both touch points moving in the same direction
                const QVector2D vectorTouchPoint1 = QVector2D (touchEvent->touchPoints().at(0).startPos() - touchEvent->touchPoints().at(0).pos());
                const QVector2D vectorTouchPoint2 = QVector2D (touchEvent->touchPoints().at(1).startPos() - touchEvent->touchPoints().at(1).pos());
                QVector2D dotProduct = vectorTouchPoint1 * vectorTouchPoint2;

                //The dot product is greater than zero if both touch points moving in the same direction
                //special case if the touch point moving exact or almost exact in x or y axis
                //one value of the dot product is zero or a little bit less than zero and
                //the other value is bigger than zero
                if (dotProduct.x() >= -500 && dotProduct.x() <= 0 && dotProduct.y() > 0) dotProduct.setX(1);
                if (dotProduct.y() >= -500 && dotProduct.y() <= 0 && dotProduct.x() > 0) dotProduct.setY(1);

                if (dotProduct.x() > 0 && dotProduct.y() > 0) {
                    const QPointF diff = (touchEvent->touchPoints().first().lastPos() - pos);
                    state->setProperty("delta", diff);
                    d->mGestureTriggered = true;
                    return TriggerGesture;
                } else {
                    //special case if the user makes a very slow pan gesture, the vectors a very short. Sometimes the
                    //dot product is then zero, so we only want to cancel the gesture if the user makes a bigger wrong gesture
                    if (vectorTouchPoint1.toPoint().manhattanLength() > 50 || vectorTouchPoint2.toPoint().manhattanLength() > 50 ) {
                        d->mGestureTriggered = false;
                        return CancelGesture;
                    } else {
                        const QPointF diff = (touchEvent->touchPoints().first().lastPos() - pos);
                        state->setProperty("delta", diff);
                        d->mGestureTriggered = true;
                        return TriggerGesture;
                    }
                }
            } else {
                state->setProperty("delta", QPointF (0, 0));
                state->setProperty("delayActive", true);
                d->mGestureTriggered = false;
                return TriggerGesture;
            }
        }
        break;
    }

    case QEvent::TouchEnd: {
        auto* touchEvent = static_cast<QTouchEvent*>(event);
        if (d->mGestureTriggered) {
            d->mGestureTriggered = false;
            state->setHotSpot(touchEvent->touchPoints().first().screenPos());
            return FinishGesture;
        }
        break;
    }

    default:
        return Ignore;
    }
    return Ignore;
}

TwoFingerPan::TwoFingerPan(QObject* parent)
    : QGesture(parent)
{
}

} // namespace
