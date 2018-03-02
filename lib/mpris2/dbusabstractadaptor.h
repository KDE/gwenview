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

#ifndef DBUSABSTRACTADAPTOR_H
#define DBUSABSTRACTADAPTOR_H

// Qt
#include <QDBusAbstractAdaptor>
#include <QVariantMap>

namespace Gwenview
{

/**
 * Extension of QDBusAbstractAdaptor for proper signalling of D-Bus object property changes
 *
 * QDBusAbstractAdaptor seems to fail on mapping QObject properties
 * to D-Bus object properties when it comes to signalling changes to a property.
 * The NOTIFY entry of Q_PROPERTY is not turned into respective D-Bus signalling of a
 * property change. So we have to do this explicitly ourselves, instead of using a normal
 * QObject signal and expecting the adaptor to translate it.
 *
 * To reduce D-Bus traffic, all registered property changes are accumulated and squashed
 * between event loops where then the D-Bus signal is emitted.
 */
class DBusAbstractAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT

public:
    /**
     * Ideally we could query the D-Bus path of the object when used, but no idea yet how to do that.
     * So one has to additionally pass here the D-Bus path at which the object is registered
     * for which this interface is added.
     *
     * @param objectDBusPath D-Bus name of the property
     * @param parent memory management parent or nullptr
     */
    explicit DBusAbstractAdaptor(const QString &objectDBusPath, QObject *parent);

protected:
    /**
     * @param propertyName D-Bus name of the property
     * @param value the new value of the property
     */
    void signalPropertyChange(const QString &propertyName, const QVariant &value);

private Q_SLOTS:
    void emitPropertiesChangeDBusSignal();

private:
    QVariantMap mChangedProperties;
    const QString mObjectPath;
};

}

#endif // DBUSABSTRACTADAPTOR_H
