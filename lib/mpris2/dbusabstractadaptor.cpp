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

#include "dbusabstractadaptor.h"

// Qt
#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaClassInfo>

namespace Gwenview
{
DBusAbstractAdaptor::DBusAbstractAdaptor(const QString &objectDBusPath, QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , mObjectPath(objectDBusPath)
{
    Q_ASSERT(!mObjectPath.isEmpty());
}

void DBusAbstractAdaptor::signalPropertyChange(const QString &propertyName, const QVariant &value)
{
    const bool firstChange = mChangedProperties.isEmpty();

    mChangedProperties.insert(propertyName, value);

    if (firstChange) {
        // trigger signal emission on next event loop
        QMetaObject::invokeMethod(this, &DBusAbstractAdaptor::emitPropertiesChangeDBusSignal, Qt::QueuedConnection);
    }
}

void DBusAbstractAdaptor::emitPropertiesChangeDBusSignal()
{
    if (mChangedProperties.isEmpty()) {
        return;
    }

    const QMetaObject *metaObject = this->metaObject();
    const int dBusInterfaceNameIndex = metaObject->indexOfClassInfo("D-Bus Interface");
    Q_ASSERT(dBusInterfaceNameIndex >= 0);
    const char *dBusInterfaceName = metaObject->classInfo(dBusInterfaceNameIndex).value();

    QDBusMessage signalMessage =
        QDBusMessage::createSignal(mObjectPath, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("PropertiesChanged"));
    signalMessage << dBusInterfaceName << mChangedProperties << QStringList();

    QDBusConnection::sessionBus().send(signalMessage);

    mChangedProperties.clear();
}

}
