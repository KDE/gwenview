/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#ifndef EVENTWATCHER_H
#define EVENTWATCHER_H

// Qt
#include <QEvent>
#include <QList>
#include <QObject>

// Local
#include "gwenviewlib_export.h"

namespace Gwenview
{

/**
 * This class emits a signal when some events are triggered on a watched
 * object.
 */
class GWENVIEWLIB_EXPORT EventWatcher : public QObject
{
    Q_OBJECT
public:
    EventWatcher(QObject* watched, const QList<QEvent::Type>& eventTypes);

    static EventWatcher* install(QObject* watched, const QList<QEvent::Type>& eventTypes, QObject* receiver, const char* slot);

    static EventWatcher* install(QObject* watched, QEvent::Type eventType, QObject* receiver, const char* slot);

Q_SIGNALS:
    void eventTriggered(QEvent*);

protected:
    bool eventFilter(QObject*, QEvent* event) override;

private:
    QList<QEvent::Type> mEventTypes;
};

}

#endif /* EVENTWATCHER_H */
