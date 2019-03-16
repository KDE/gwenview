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
#ifndef TOUCH_H
#define TOUCH_H

#include <lib/gwenviewlib_export.h>
// Qt
#include <QObject>

// KDE

// Local
#include "lib/touch/tapholdandmoving.h"
#include "lib/touch/twofingerpan.h"
#include "lib/touch/oneandtwofingerswipe.h"
#include "lib/touch/doubletap.h"
#include "lib/touch/twofingertap.h"

namespace Gwenview
{

struct TouchPrivate;
class GWENVIEWLIB_EXPORT Touch : public QObject
{
    Q_OBJECT
public:
    Touch(QObject* target);
    ~Touch() override;
    void setZoomParameter (qreal, qreal);
    void setRotationThreshold (qreal);
    qreal getRotationFromPinchGesture(QGestureEvent*);
    qreal getZoomFromPinchGesture(QGestureEvent*);
    QPoint positionGesture(QGestureEvent*);
    bool checkTwoFingerPanGesture(QGestureEvent*);
    bool checkOneAndTwoFingerSwipeGesture(QGestureEvent*);
    bool checkTapGesture(QGestureEvent*);
    bool checkDoubleTapGesture(QGestureEvent*);
    bool checkTwoFingerTapGesture(QGestureEvent*);
    bool checkTapHoldAndMovingGesture(QGestureEvent*, QObject*);
    bool checkPinchGesture(QGestureEvent*);
    void touchToMouseRelease(QPoint, QObject*);
    void touchToMouseMove(QPoint, QEvent*, Qt::MouseButton);
    void touchToMouseMove(QPoint, QObject*, Qt::MouseButton);
    void touchToMouseClick(QPoint, QObject*);
    void setPanGestureState ( QGestureEvent* event );
    Qt::GestureState getLastPanGestureState();
    QPointF getLastTapPos();
    void setLastTapPos(QPointF);
    bool getTapHoldandMovingGestureActive();
    void setTapHoldandMovingGestureActive(bool);

    Qt::GestureType getTapHoldandMovingGesture();
    Qt::GestureType getTwoFingerPanGesture();
    Qt::GestureType getOneAndTwoFingerSwipeGesture();
    Qt::GestureType getDoubleTapGesture();
    Qt::GestureType getTwoFingerTapGesture();

protected:
    bool eventFilter(QObject*, QEvent*) override;


signals:
    void PanTriggered(const QPointF&);
    void swipeLeftTriggered();
    void swipeRightTriggered();
    void doubleTapTriggered();
    void tapHoldAndMovingTriggered(const QPoint&);
    void tapTriggered(const QPoint&);
    void pinchGestureStarted();
    void twoFingerTapTriggered();
    void pinchZoomTriggered(qreal, const QPoint&);
    void pinchRotateTriggered(qreal);

private:
    qreal calculateZoom (qreal, qreal);
    void touchToMouseEvent(QPoint, QObject*, QEvent::Type, Qt::MouseButton, Qt::MouseButtons);
    bool gestureEvent(QGestureEvent*);
    TouchPrivate* const d;
};

} // namespace
#endif /* TOUCH_H */
