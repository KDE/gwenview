/*
 * Copyright (C) 2011 Canonical, Ltd.
 *
 * Authors:
 *  Florian Boucault <florian.boucault@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "propertybinder.h"

#include <QMetaObject>

PropertyBinder::PropertyBinder(QObject *parent) :
    QObject(parent), m_objectA(NULL), m_objectB(NULL)
{
}

static const char* signalNameFromMethod(QMetaMethod* signalMetaMethod)
{
    /* Prepend QSIGNAL_CODE to the signature of the method mimicking
       the behaviour of the SIGNAL macro.
       See src/corelib/kernel/qobjectdefs.h in Qt's source.
    */
    QString signalName = signalMetaMethod->signature();
    signalName.prepend(QSIGNAL_CODE);
    return signalName.toAscii().data();
}

bool PropertyBinder::bind(QObject* objectA, const char* propertyA,
                          QObject* objectB, const char* propertyB)
{
    bool success;
    const QMetaObject* metaObjectA = objectA->metaObject();
    const QMetaObject* metaObjectB = objectB->metaObject();

    m_metaPropertyA = metaObjectA->property(metaObjectA->indexOfProperty(propertyA));
    m_metaPropertyB = metaObjectB->property(metaObjectB->indexOfProperty(propertyB));

    /* QObject does not yet provide a way to connect QMetaMethods.
       Compute manually the right signal names instead.
       Fixed in Qt 4.8.
       See http://bugreports.qt.nokia.com/browse/QTBUG-10637
    */
    success = connect(objectA, signalNameFromMethod(&m_metaPropertyA.notifySignal()), SLOT(copyAintoB()));
    if (!success) {
        return false;
    }
    success = connect(objectB, signalNameFromMethod(&m_metaPropertyB.notifySignal()), SLOT(copyBintoA()));
    if (!success) {
        objectA->disconnect(this);
        return false;
    }

    m_objectA = objectA;
    m_objectB = objectB;

    return true;
}

void PropertyBinder::unbind()
{
    if (m_objectA != NULL) {
        m_objectA->disconnect(this);
        m_objectA = NULL;
    }

    if (m_objectB != NULL) {
        m_objectB->disconnect(this);
        m_objectB = NULL;
    }
}

void PropertyBinder::copyAintoB()
{
    QVariant valueA = m_metaPropertyA.read(m_objectA);
    m_metaPropertyB.write(m_objectB, valueA);
}

void PropertyBinder::copyBintoA()
{
    QVariant valueB = m_metaPropertyB.read(m_objectB);
    m_metaPropertyA.write(m_objectA, valueB);
}

#include "propertybinder.moc"
