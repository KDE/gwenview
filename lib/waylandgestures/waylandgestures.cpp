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

#include "qwayland-pointer-gestures-unstable-v1.h"
#include <QGuiApplication>
#include <QtWaylandClient/qwaylandclientextension.h>
#include <qnamespace.h>
#include <qpa/qplatformnativeinterface.h>
#include <wayland-util.h>

#include "wayland-pointer-gestures-unstable-v1-client-protocol.h"
#include "waylandgestures.h"

using namespace Gwenview;

class PointerGestures : public QWaylandClientExtensionTemplate<PointerGestures>, public QtWayland::zwp_pointer_gestures_v1
{
public:
    PointerGestures()
        : QWaylandClientExtensionTemplate<PointerGestures>(3)
    {
    }
};

class PinchGesture : public QObject, public QtWayland::zwp_pointer_gesture_pinch_v1
{
    Q_OBJECT
public:
public:
    PinchGesture(struct ::zwp_pointer_gesture_pinch_v1 *object, QObject *parent)
        : QObject(parent)
        , zwp_pointer_gesture_pinch_v1(object)
    {
    }

Q_SIGNALS:
    void gestureBegin(uint32_t serial, uint32_t time, uint32_t fingers);
    void gestureUpdate(uint32_t time, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation);
    void gestureEnd(uint32_t serial, uint32_t time, int32_t cancelled);

private:
    virtual void zwp_pointer_gesture_pinch_v1_begin(uint32_t serial, uint32_t time, struct ::wl_surface *surface, uint32_t fingers) override
    {
        Q_UNUSED(surface);
        Q_EMIT gestureBegin(serial, time, fingers);
    }

    virtual void zwp_pointer_gesture_pinch_v1_update(uint32_t time, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation) override
    {
        Q_EMIT gestureUpdate(time, dx, dy, scale, rotation);
    }

    virtual void zwp_pointer_gesture_pinch_v1_end(uint32_t serial, uint32_t time, int32_t cancelled) override
    {
        Q_EMIT gestureEnd(serial, time, cancelled);
    }
};

WaylandGestures::WaylandGestures(QObject *parent)
    : QObject(parent)
{
    m_startZoom = 1.0;
    m_zoomModifier = 1.0;
    m_pointerGestures = new PointerGestures();
    connect(m_pointerGestures, &PointerGestures::activeChanged, this, [this]() {
        init();
    });
}

WaylandGestures::~WaylandGestures()
{
    if (m_pointerGestures) {
        m_pointerGestures->release();
    }
    if (m_pinchGesture) {
        m_pinchGesture->destroy();
    }
}

void WaylandGestures::setStartZoom(double startZoom)
{
    m_startZoom = startZoom;
}

void WaylandGestures::init()
{
    QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
    if (!native) {
        return;
    }

    const auto pointer = reinterpret_cast<wl_pointer *>(native->nativeResourceForIntegration(QByteArrayLiteral("wl_pointer")));
    if (!pointer) {
        return;
    }

    m_pinchGesture = new PinchGesture(m_pointerGestures->get_pinch_gesture(pointer), this);

    connect(m_pinchGesture, &PinchGesture::gestureBegin, this, [this]() {
        Q_EMIT pinchGestureStarted();
    });

    connect(m_pinchGesture, &PinchGesture::gestureUpdate, this, [this](uint32_t time, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation) {
        Q_EMIT pinchZoomChanged(m_startZoom * wl_fixed_to_double(scale) * m_zoomModifier);
    });
}

#include "waylandgestures.moc"
