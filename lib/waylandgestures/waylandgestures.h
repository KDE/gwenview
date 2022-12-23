/*
Gwenview: an image viewer
Copyright 2019 Steffen Hartleib <steffenhartleib@t-online.de>
Copyright 2022 Carl Schwan <carlschwan@kde.org>
Copyright 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

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
#pragma once

#include <lib/gwenviewlib_export.h>

#include <QObject>

class PointerGestures;
class PinchGesture;

namespace Gwenview
{
class GWENVIEWLIB_EXPORT WaylandGestures : public QObject
{
    Q_OBJECT
public:
    WaylandGestures(QObject *parent = nullptr);
    ~WaylandGestures();
    void init();
    void setStartZoom(qreal);
    void setZoomModifier(qreal);
    void setRotationThreshold(qreal);

Q_SIGNALS:
    void pinchGestureStarted();
    void pinchZoomChanged(double);

private:
    PointerGestures *m_pointerGestures;
    PinchGesture *m_pinchGesture;
    double m_startZoom;
    double m_zoomModifier;
};

} // namespace
