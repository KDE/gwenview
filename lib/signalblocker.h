// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef SIGNALBLOCKER_H
#define SIGNALBLOCKER_H

#include <QObject>

namespace Gwenview
{

/**
 * An RAII class to block and unblock signals from a QObject instance
 */
class SignalBlocker
{
public:
    explicit SignalBlocker(QObject* object)
    {
        mObject = object;
        mWasBlocked = object->blockSignals(true);
    }

    ~SignalBlocker()
    {
        mObject->blockSignals(mWasBlocked);
    }

private:
    QObject* mObject;
    bool mWasBlocked;
};

} // namespace

#endif /* SIGNALBLOCKER_H */
