/*
Gwenview: an image viewer
Copyright 2013 Martin Gräßlin <mgraesslin@kde.org>
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

#include "lockscreenwatcher.h"

// lib
#include <screensaverdbusinterface.h>
// Qt
#include <QDBusServiceWatcher>
#include <QFutureWatcher>
#include <QtConcurrentRun>

namespace Gwenview
{
using DBusBoolReplyWatcher = QFutureWatcher<QDBusReply<bool>>;
using DBusStringReplyWatcher = QFutureWatcher<QDBusReply<QString>>;

inline QString screenSaverServiceName()
{
    return QStringLiteral("org.freedesktop.ScreenSaver");
}

LockScreenWatcher::LockScreenWatcher(QObject *parent)
    : QObject(parent)
{
    auto screenLockServiceWatcher = new QDBusServiceWatcher(this);
    connect(screenLockServiceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &LockScreenWatcher::onScreenSaverServiceOwnerChanged);
    screenLockServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
    screenLockServiceWatcher->addWatchedService(screenSaverServiceName());

    if (QDBusConnection::sessionBus().interface()) {
        auto watcher = new DBusBoolReplyWatcher(this);
        connect(watcher, &DBusBoolReplyWatcher::finished, this, &LockScreenWatcher::onServiceRegisteredQueried);
        connect(watcher, &DBusBoolReplyWatcher::canceled, watcher, &DBusBoolReplyWatcher::deleteLater);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        watcher->setFuture(
            QtConcurrent::run(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::isServiceRegistered, screenSaverServiceName()));
#else
        watcher->setFuture(
            QtConcurrent::run(&QDBusConnectionInterface::isServiceRegistered, QDBusConnection::sessionBus().interface(), screenSaverServiceName()));
#endif
    }
}

LockScreenWatcher::~LockScreenWatcher() = default;

bool LockScreenWatcher::isLocked() const
{
    return mLocked;
}

void LockScreenWatcher::onScreenSaverServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)

    if (serviceName != screenSaverServiceName()) {
        return;
    }

    delete mScreenSaverInterface;
    mScreenSaverInterface = nullptr;

    if (!newOwner.isEmpty()) {
        mScreenSaverInterface = new OrgFreedesktopScreenSaverInterface(newOwner, QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus(), this);
        connect(mScreenSaverInterface, &OrgFreedesktopScreenSaverInterface::ActiveChanged, this, &LockScreenWatcher::onScreenSaverActiveChanged);

        auto watcher = new QDBusPendingCallWatcher(mScreenSaverInterface->GetActive(), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &LockScreenWatcher::onActiveQueried);
    } else {
        if (mLocked) {
            // reset
            mLocked = false;
            Q_EMIT isLockedChanged(mLocked);
        }
    }
}

void LockScreenWatcher::onServiceRegisteredQueried()
{
    auto watcher = dynamic_cast<DBusBoolReplyWatcher *>(sender());
    if (!watcher) {
        return;
    }

    const QDBusReply<bool> &reply = watcher->result();

    if (reply.isValid() && reply.value()) {
        auto ownerWatcher = new DBusStringReplyWatcher(this);
        connect(ownerWatcher, &DBusStringReplyWatcher::finished, this, &LockScreenWatcher::onServiceOwnerQueried);
        connect(ownerWatcher, &DBusStringReplyWatcher::canceled, ownerWatcher, &DBusStringReplyWatcher::deleteLater);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        ownerWatcher->setFuture(
            QtConcurrent::run(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceOwner, screenSaverServiceName()));
#else
        ownerWatcher->setFuture(
            QtConcurrent::run(&QDBusConnectionInterface::serviceOwner, QDBusConnection::sessionBus().interface(), screenSaverServiceName()));
#endif
    }

    watcher->deleteLater();
}

void LockScreenWatcher::onServiceOwnerQueried()
{
    auto watcher = dynamic_cast<DBusStringReplyWatcher *>(sender());
    if (!watcher) {
        return;
    }

    const QDBusReply<QString> reply = watcher->result();

    if (reply.isValid()) {
        onScreenSaverServiceOwnerChanged(screenSaverServiceName(), QString(), reply.value());
    }

    watcher->deleteLater();
}

void LockScreenWatcher::onActiveQueried(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<bool> reply = *watcher;
    if (!reply.isError()) {
        onScreenSaverActiveChanged(reply.value());
    }

    watcher->deleteLater();
}

void LockScreenWatcher::onScreenSaverActiveChanged(bool isActive)
{
    if (mLocked == isActive) {
        return;
    }

    mLocked = isActive;

    Q_EMIT isLockedChanged(mLocked);
}

}
