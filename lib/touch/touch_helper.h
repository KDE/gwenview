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
GNU General Public License for more detai
ls.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef TOUCH_HELPER_H
#define TOUCH_HELPER_H

// Qt
#include <QObject>
// KDE

// Local
#include <lib/gwenviewlib_export.h>

class QPoint;
class QPointF;
class QEvent;

namespace Gwenview
{

namespace Touch_Helper
{

//constant variables that define touch behavior
struct Touch
{
    //a little delay in the begin of the gesture, to get more data of the touch points moving so the recognizing
    //of the pan gesture is better
    static const int gestureDelay = 110;
    //this defines how much a touch point can move, for a single tap gesture
    static const int wiggleRoomForTap = 10;
    //how long must a touch point be stationary, before he can move for a TabHoldAndMoving gesture
    static const int durationForTapHold = 400;
    //in what time and how far must the touch point moving to trigger a swipe gesture
    static const int maxTimeFrameForSwipe = 100;
    static const int minDistanceForSwipe = 70;
    //How long is the duration for a simple tap gesture
    static const int maxTimeForTap = 100;
    //max interval for a double tap gesture
    static const int doubleTapInterval = 400;
};

GWENVIEWLIB_EXPORT QPoint simpleTapPosition(QEvent*);
GWENVIEWLIB_EXPORT QPoint simpleTouchPosition(QEvent*,int = 0);
GWENVIEWLIB_EXPORT bool touchStationary(QEvent*);

} // namespace
} // namespace

#endif /* TOUCH_HELPER_H */
