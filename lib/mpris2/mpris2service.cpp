/*
Gwenview: an image viewer
Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "mpris2service.h"

// lib
#include "lockscreenwatcher.h"
#include "mprismediaplayer2.h"
#include "mprismediaplayer2player.h"
#include <slideshow.h>
// Qt
#include <QDBusConnection>
// std
#include <unistd.h>

namespace Gwenview
{
inline QString mediaPlayer2ObjectPath()
{
    return QStringLiteral("/org/mpris/MediaPlayer2");
}

Mpris2Service::Mpris2Service(SlideShow *slideShow,
                             ContextManager *contextManager,
                             QAction *toggleSlideShowAction,
                             QAction *fullScreenAction,
                             QAction *previousAction,
                             QAction *nextAction,
                             QObject *parent)
    : QObject(parent)
{
    new MprisMediaPlayer2(mediaPlayer2ObjectPath(), fullScreenAction, this);
    new MprisMediaPlayer2Player(mediaPlayer2ObjectPath(), slideShow, contextManager, toggleSlideShowAction, fullScreenAction, previousAction, nextAction, this);

    // To avoid appearing in the media controller on the lock screen,
    // which might be not expected or wanted for Gwenview,
    // the MPRIS service is unregistered while the lockscreen is active.
    auto lockScreenWatcher = new LockScreenWatcher(this);
    connect(lockScreenWatcher, &LockScreenWatcher::isLockedChanged, this, &Mpris2Service::onLockScreenLockedChanged);

    if (!lockScreenWatcher->isLocked()) {
        registerOnDBus();
    }
}

Mpris2Service::~Mpris2Service()
{
    unregisterOnDBus();
}

void Mpris2Service::registerOnDBus()
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    // try to register MPRIS presentation object
    // to be done before registering the service name, so it is already present
    // when controllers react to the service name having appeared
    const bool objectRegistered = sessionBus.registerObject(mediaPlayer2ObjectPath(), this, QDBusConnection::ExportAdaptors);

    // try to register MPRIS presentation service
    if (objectRegistered) {
        mMpris2ServiceName = QStringLiteral("org.mpris.MediaPlayer2.Gwenview");

        bool serviceRegistered = QDBusConnection::sessionBus().registerService(mMpris2ServiceName);

        // Perhaps not the first instance? Try again with another name, as specified by MPRIS2 spec:
        if (!serviceRegistered) {
            mMpris2ServiceName = mMpris2ServiceName + QLatin1String(".instance") + QString::number(getpid());
            serviceRegistered = QDBusConnection::sessionBus().registerService(mMpris2ServiceName);
        }
        if (!serviceRegistered) {
            mMpris2ServiceName.clear();
            sessionBus.unregisterObject(mediaPlayer2ObjectPath());
        }
    }
}

void Mpris2Service::unregisterOnDBus()
{
    if (mMpris2ServiceName.isEmpty()) {
        return;
    }

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    sessionBus.unregisterService(mMpris2ServiceName);
    sessionBus.unregisterObject(mediaPlayer2ObjectPath());
}

void Mpris2Service::onLockScreenLockedChanged(bool isLocked)
{
    if (isLocked) {
        unregisterOnDBus();
    } else {
        registerOnDBus();
    }
}

}
