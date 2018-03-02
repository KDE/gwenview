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

#ifndef LOCKSCREENWATCHER_H
#define LOCKSCREENWATCHER_H

// Qt
#include <QObject>

class OrgFreedesktopScreenSaverInterface;
class QDBusPendingCallWatcher;

namespace Gwenview
{

class LockScreenWatcher : public QObject
{
    Q_OBJECT

public:
    explicit LockScreenWatcher(QObject *parent);
    ~LockScreenWatcher() override;

public:
    bool isLocked() const;

Q_SIGNALS:
    void isLockedChanged(bool locked);

private:
    void onScreenSaverActiveChanged(bool isActive);
    void onActiveQueried(QDBusPendingCallWatcher *watcher);
    void onScreenSaverServiceOwnerChanged(const QString &serviceName,
                                          const QString &oldOwner, const QString &newOwner);
    void onServiceRegisteredQueried();
    void onServiceOwnerQueried();

private:
    OrgFreedesktopScreenSaverInterface *mScreenSaverInterface;
    bool mLocked;
};

}

#endif
