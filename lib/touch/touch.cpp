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
#include "touch.h"

// STL
#include <cmath>

// Qt
#include <QCoreApplication>
#include <QDateTime>
#include <QGraphicsWidget>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QStyleHints>
#include <QWidget>

// KF

// Local
#include "gwenview_lib_debug.h"
#include "touch_helper.h"

namespace Gwenview
{
struct TouchPrivate {
    Touch *q = nullptr;
    QObject *mTarget = nullptr;
    Qt::GestureState mLastPanGestureState;
    QPointF mLastTapPos;
    bool mTabHoldandMovingGestureActive;
    qreal mStartZoom;
    qreal mZoomModifier;
    qreal mRotationThreshold;
    qint64 mLastTouchTimeStamp;

    TapHoldAndMovingRecognizer *mTapHoldAndMovingRecognizer = nullptr;
    Qt::GestureType mTapHoldAndMoving;
    TwoFingerPanRecognizer *mTwoFingerPanRecognizer = nullptr;
    Qt::GestureType mTwoFingerPan;
    OneAndTwoFingerSwipeRecognizer *mOneAndTwoFingerSwipeRecognizer = nullptr;
    Qt::GestureType mOneAndTwoFingerSwipe;
    DoubleTapRecognizer *mDoubleTapRecognizer = nullptr;
    Qt::GestureType mDoubleTap;
    TwoFingerTapRecognizer *mTwoFingerTapRecognizer = nullptr;
    Qt::GestureType mTwoFingerTap;
};

Touch::Touch(QObject *target)
    : QObject()
    , d(new TouchPrivate)
{
    d->q = this;
    d->mTarget = target;

    d->mTapHoldAndMovingRecognizer = new TapHoldAndMovingRecognizer();
    d->mTapHoldAndMoving = QGestureRecognizer::registerRecognizer(d->mTapHoldAndMovingRecognizer);

    d->mTwoFingerPanRecognizer = new TwoFingerPanRecognizer();
    d->mTwoFingerPan = QGestureRecognizer::registerRecognizer(d->mTwoFingerPanRecognizer);

    d->mTwoFingerTapRecognizer = new TwoFingerTapRecognizer();
    d->mTwoFingerTap = QGestureRecognizer::registerRecognizer(d->mTwoFingerTapRecognizer);

    d->mOneAndTwoFingerSwipeRecognizer = new OneAndTwoFingerSwipeRecognizer();
    d->mOneAndTwoFingerSwipe = QGestureRecognizer::registerRecognizer(d->mOneAndTwoFingerSwipeRecognizer);

    d->mDoubleTapRecognizer = new DoubleTapRecognizer();
    d->mDoubleTap = QGestureRecognizer::registerRecognizer(d->mDoubleTapRecognizer);

    if (qobject_cast<QGraphicsWidget *>(target)) {
        auto widgetTarget = qobject_cast<QGraphicsWidget *>(target);
        widgetTarget->grabGesture(d->mOneAndTwoFingerSwipe);
        widgetTarget->grabGesture(d->mDoubleTap);
        widgetTarget->grabGesture(Qt::TapGesture);
        widgetTarget->grabGesture(Qt::PinchGesture);
        widgetTarget->grabGesture(d->mTwoFingerTap);
        widgetTarget->grabGesture(d->mTwoFingerPan);
        widgetTarget->grabGesture(d->mTapHoldAndMoving);
    } else if (qobject_cast<QWidget *>(target)) {
        QWidget *widgetTarget = qobject_cast<QWidget *>(target);
        widgetTarget->grabGesture(Qt::TapGesture);
        widgetTarget->grabGesture(Qt::PinchGesture);
        widgetTarget->grabGesture(d->mTwoFingerTap);
        widgetTarget->grabGesture(d->mTwoFingerPan);
        widgetTarget->grabGesture(d->mTapHoldAndMoving);
    }
    target->installEventFilter(this);
}

Touch::~Touch()
{
    delete d;
}

bool Touch::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::TouchBegin) {
        d->mLastTouchTimeStamp = QDateTime::currentMSecsSinceEpoch();
        const QPoint pos = Touch_Helper::simpleTouchPosition(event);
        touchToMouseMove(pos, event, Qt::NoButton);
        return true;
    }
    if (event->type() == QEvent::TouchUpdate) {
        auto touchEvent = static_cast<QTouchEvent *>(event);
        d->mLastTouchTimeStamp = QDateTime::currentMSecsSinceEpoch();
        // because we suppress the making of mouse event through Qt, we need to make our own one finger panning
        // but only if no TapHoldandMovingGesture is active (Drag and Drop action)
        if (touchEvent->touchPoints().size() == 1 && !getTapHoldandMovingGestureActive()) {
            const QPointF delta = touchEvent->touchPoints().first().lastPos() - touchEvent->touchPoints().first().pos();
            Q_EMIT PanTriggered(delta);
        }
        return true;
    }
    if (event->type() == QEvent::TouchEnd) {
        d->mLastTouchTimeStamp = QDateTime::currentMSecsSinceEpoch();
    }
    if (event->type() == QEvent::Gesture) {
        gestureEvent(static_cast<QGestureEvent *>(event));
    }
    return false;
}

bool Touch::gestureEvent(QGestureEvent *event)
{
    bool ret = false;

    if (checkTwoFingerTapGesture(event)) {
        ret = true;
    }

    if (checkPinchGesture(event)) {
        ret = true;
        Q_EMIT pinchZoomTriggered(getZoomFromPinchGesture(event), positionGesture(event), d->mLastTouchTimeStamp);
        Q_EMIT pinchRotateTriggered(getRotationFromPinchGesture(event));
    }

    if (checkTapGesture(event)) {
        ret = true;
        if (event->widget()) {
            touchToMouseClick(positionGesture(event), event->widget());
        }
        Q_EMIT tapTriggered(positionGesture(event));
    }

    if (checkTapHoldAndMovingGesture(event, d->mTarget)) {
        ret = true;
        Q_EMIT tapHoldAndMovingTriggered(positionGesture(event));
    }

    if (checkDoubleTapGesture(event)) {
        ret = true;
    }

    if (checkOneAndTwoFingerSwipeGesture(event)) {
        ret = true;
    }

    checkTwoFingerPanGesture(event);

    return ret;
}

void Touch::setZoomParameter(qreal modifier, qreal startZoom)
{
    d->mZoomModifier = modifier;
    d->mStartZoom = startZoom;
}

void Touch::setRotationThreshold(qreal rotationThreshold)
{
    d->mRotationThreshold = rotationThreshold;
}

qreal Touch::getRotationFromPinchGesture(QGestureEvent *event)
{
    const QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture));
    static qreal lastRotationAngel;
    if (pinch) {
        if (pinch->state() == Qt::GestureStarted) {
            lastRotationAngel = 0;
            return 0.0;
        }
        if (pinch->state() == Qt::GestureUpdated) {
            const qreal rotationDelta = pinch->rotationAngle() - pinch->lastRotationAngle();
            // very low and high changes in the rotation are suspect, so we ignore them
            if (abs(rotationDelta) <= 1.5 || abs(rotationDelta) >= 30) {
                return 0.0;
            }
            lastRotationAngel += rotationDelta;
            const qreal ret = lastRotationAngel;
            if (abs(lastRotationAngel) > d->mRotationThreshold) {
                lastRotationAngel = 0;
                return ret;
            } else {
                return 0.0;
            }
        }
    }
    return 0.0;
}

qreal Touch::getZoomFromPinchGesture(QGestureEvent *event)
{
    static qreal lastZoom;
    const QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture));
    if (pinch) {
        if (pinch->state() == Qt::GestureStarted) {
            lastZoom = d->mStartZoom;
            return -1;
        }
        if (pinch->state() == Qt::GestureUpdated) {
            lastZoom = calculateZoom(pinch->scaleFactor(), d->mZoomModifier) * lastZoom;
            return lastZoom;
        }
    }
    return -1;
}

qreal Touch::calculateZoom(qreal scale, qreal modifier)
{
    return ((scale - 1.0) * modifier) + 1.0;
}

QPoint Touch::positionGesture(QGestureEvent *event)
{
    // return the position or the center point for follow gestures: QTapGesture, TabHoldAndMovingGesture and PinchGesture;
    QPoint position = QPoint(-1, -1);
    if (auto tap = static_cast<QTapGesture *>(event->gesture(Qt::TapGesture))) {
        position = tap->position().toPoint();
    } else if (QGesture *gesture = event->gesture(getTapHoldandMovingGesture())) {
        position = gesture->property("pos").toPoint();
    } else if (auto pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        if (qobject_cast<QGraphicsWidget *>(d->mTarget)) {
            auto widget = qobject_cast<QGraphicsWidget *>(d->mTarget);
            position = widget->mapFromScene(event->mapToGraphicsScene(pinch->centerPoint())).toPoint();
        } else {
            position = pinch->centerPoint().toPoint();
        }
    }
    return position;
}

bool Touch::checkTwoFingerPanGesture(QGestureEvent *event)
{
    if (QGesture *gesture = event->gesture(getTwoFingerPanGesture())) {
        event->accept();
        setPanGestureState(event);
        if (gesture->state() == Qt::GestureUpdated) {
            const QPoint diff = gesture->property("delta").toPoint();
            Q_EMIT PanTriggered(diff);
            return true;
        }
    }
    return false;
}

bool Touch::checkOneAndTwoFingerSwipeGesture(QGestureEvent *event)
{
    if (QGesture *gesture = event->gesture(getOneAndTwoFingerSwipeGesture())) {
        event->accept();
        if (gesture->state() == Qt::GestureFinished) {
            if (gesture->property("right").toBool()) {
                Q_EMIT swipeRightTriggered();
                return true;
            } else if (gesture->property("left").toBool()) {
                Q_EMIT swipeLeftTriggered();
                return true;
            }
        }
    }
    return false;
}

bool Touch::checkTapGesture(QGestureEvent *event)
{
    const QTapGesture *tap = static_cast<QTapGesture *>(event->gesture(Qt::TapGesture));
    if (tap) {
        event->accept();
        if (tap->state() == Qt::GestureFinished)
            return true;
    }
    return false;
}

bool Touch::checkDoubleTapGesture(QGestureEvent *event)
{
    if (QGesture *gesture = event->gesture(getDoubleTapGesture())) {
        event->accept();
        if (gesture->state() == Qt::GestureFinished) {
            Q_EMIT doubleTapTriggered();
            return true;
        }
    }
    return false;
}

bool Touch::checkTwoFingerTapGesture(QGestureEvent *event)
{
    if (QGesture *twoFingerTap = event->gesture(getTwoFingerTapGesture())) {
        event->accept();
        if (twoFingerTap->state() == Qt::GestureFinished) {
            Q_EMIT twoFingerTapTriggered();
            return true;
        }
    }
    return false;
}

bool Touch::checkTapHoldAndMovingGesture(QGestureEvent *event, QObject *target)
{
    if (QGesture *tapHoldAndMoving = event->gesture(getTapHoldandMovingGesture())) {
        event->accept();
        const QPoint pos = tapHoldAndMoving->property("pos").toPoint();
        switch (tapHoldAndMoving->state()) {
        case Qt::GestureStarted: {
            setTapHoldandMovingGestureActive(true);
            return true;
        }
        case Qt::GestureUpdated: {
            touchToMouseMove(pos, target, Qt::LeftButton);
            break;
        }
        case Qt::GestureCanceled:
        case Qt::GestureFinished: {
            touchToMouseRelease(pos, target);
            setTapHoldandMovingGestureActive(false);
            break;
        }
        default:
            break;
        }
    }
    return false;
}

bool Touch::checkPinchGesture(QGestureEvent *event)
{
    static qreal lastScaleFactor;
    const QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture));
    if (pinch) {
        // we don't want a pinch gesture, if a pan gesture is active
        // only exception is, if the pinch gesture state is Qt::GestureStarted
        if (getLastPanGestureState() == Qt::GestureCanceled || pinch->state() == Qt::GestureStarted) {
            event->accept();
            if (pinch->state() == Qt::GestureStarted) {
                lastScaleFactor = 0;
                Q_EMIT pinchGestureStarted(d->mLastTouchTimeStamp);
            } else if (pinch->state() == Qt::GestureUpdated) {
                // Because of a bug in Qt in a gesture event in a graphicsview, all gestures are trigger twice
                // https://bugreports.qt.io/browse/QTBUG-13103
                // the duplicate events have the same scaleFactor, so I ignore them
                if (lastScaleFactor == pinch->scaleFactor()) {
                    return false;
                } else {
                    lastScaleFactor = pinch->scaleFactor();
                }
            }
            return true;
        }
    }
    return false;
}

void Touch::touchToMouseRelease(QPoint pos, QObject *receiver)
{
    touchToMouseEvent(pos, receiver, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::LeftButton);
}

void Touch::touchToMouseMove(QPoint pos, QEvent *event, Qt::MouseButton button)
{
    if (auto touchEvent = static_cast<QTouchEvent *>(event)) {
        touchToMouseEvent(pos, touchEvent->target(), QEvent::MouseMove, button, button);
    }
}

void Touch::touchToMouseMove(QPoint pos, QObject *receiver, Qt::MouseButton button)
{
    touchToMouseEvent(pos, receiver, QEvent::MouseMove, button, button);
}

void Touch::touchToMouseClick(QPoint pos, QObject *receiver)
{
    touchToMouseEvent(pos, receiver, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
    touchToMouseEvent(pos, receiver, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::LeftButton);
}

void Touch::touchToMouseEvent(QPoint pos, QObject *receiver, QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    auto evt = new QMouseEvent(type, pos, button, buttons, Qt::NoModifier);
    QCoreApplication::postEvent(receiver, evt);
}

Qt::GestureState Touch::getLastPanGestureState()
{
    return d->mLastPanGestureState;
    ;
}

void Touch::setPanGestureState(QGestureEvent *event)
{
    if (QGesture *panGesture = event->gesture(getTwoFingerPanGesture())) {
        d->mLastPanGestureState = panGesture->state();
    }
    return;
}

QPointF Touch::getLastTapPos()
{
    return d->mLastTapPos;
}

void Touch::setLastTapPos(QPointF pos)
{
    d->mLastTapPos = pos;
}

Qt::GestureType Touch::getTapHoldandMovingGesture()
{
    return d->mTapHoldAndMoving;
}

Qt::GestureType Touch::getTwoFingerPanGesture()
{
    return d->mTwoFingerPan;
}

Qt::GestureType Touch::getOneAndTwoFingerSwipeGesture()
{
    return d->mOneAndTwoFingerSwipe;
}

Qt::GestureType Touch::getDoubleTapGesture()
{
    return d->mDoubleTap;
}

Qt::GestureType Touch::getTwoFingerTapGesture()
{
    return d->mTwoFingerTap;
}

bool Touch::getTapHoldandMovingGestureActive()
{
    return d->mTabHoldandMovingGestureActive;
}

void Touch::setTapHoldandMovingGestureActive(bool active)
{
    d->mTabHoldandMovingGestureActive = active;
}

} // namespace
