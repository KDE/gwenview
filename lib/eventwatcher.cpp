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
#include "eventwatcher.h"

namespace Gwenview
{

EventWatcher::EventWatcher(QObject* watched, const QList<QEvent::Type>& eventTypes)
: QObject(watched)
, mEventTypes(eventTypes)
{
    Q_ASSERT(watched);
    watched->installEventFilter(this);
}

EventWatcher* EventWatcher::install(QObject* watched, const QList<QEvent::Type>& eventTypes, QObject* receiver, const char* slot)
{
    EventWatcher* watcher = new EventWatcher(watched, eventTypes);
    connect(watcher, SIGNAL(eventTriggered(QEvent*)), receiver, slot);
    return watcher;
}

EventWatcher* EventWatcher::install(QObject* watched, QEvent::Type eventType, QObject* receiver, const char* slot)
{
    EventWatcher* watcher = new EventWatcher(watched, QList<QEvent::Type>() << eventType);
    connect(watcher, SIGNAL(eventTriggered(QEvent*)), receiver, slot);
    return watcher;
}

bool EventWatcher::eventFilter(QObject*, QEvent* event)
{
    if (mEventTypes.contains(event->type())) {
        emit eventTriggered(event);
    }
    return false;
}

} // namespace
