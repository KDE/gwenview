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

#ifndef PROPERTYBINDER_H
#define PROPERTYBINDER_H

#include <QObject>
#include <QMetaProperty>

/**
 * PropertyBinder helps synchronizing the values of 2 properties.
 *
 * Given 2 properties A and B of 2 QObjects, PropertyBinder::bind ensures
 * that any change in value of property A happens to property B, and vice-versa.
 *
 * Warning: the properties' values are not made equal when binding. If you wish
 * to make them equal, you ought to do so manually.
 *
 * There are bug reports pending in Qt's bug tracker requesting a similar
 * functionality:
 *
 * http://bugreports.qt.nokia.com/browse/QTBUG-15104
 * http://bugreports.qt.nokia.com/browse/QTBUG-5837
 */
class PropertyBinder : public QObject
{
    Q_OBJECT

public:
    PropertyBinder(QObject *parent = 0);

    bool bind(QObject* objectA, const char* propertyA, QObject* objectB, const char* propertyB);
    void unbind();

private Q_SLOTS:
    void copyAintoB();
    void copyBintoA();

private:
    QObject* m_objectA;
    QObject* m_objectB;

    QMetaProperty m_metaPropertyA;
    QMetaProperty m_metaPropertyB;
};

#endif // PROPERTYBINDER_H
