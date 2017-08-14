// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/

#include "fitsplugin.h"

#include "fitshandler.h"

#include <QImageIOHandler>
#include <QtPlugin>

QStringList FitsPlugin::keys() const
{
    return QStringList() << QLatin1String("fits") << QLatin1String("fit");
}

QImageIOPlugin::Capabilities FitsPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "fits" || format == "fit") {
        return Capabilities(CanRead);
    }
    if (!format.isEmpty()) {
        return 0;
    }
    if (!device->isOpen()) {
        return 0;
    }

    Capabilities cap;

    if (device->isReadable()) {
        Gwenview::FitsHandler handler;

        handler.setDevice(device);
        handler.setFormat(format);
        if (handler.canRead()) {
            cap |= CanRead;
        }
    }
    return cap;
}

QImageIOHandler *FitsPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new Gwenview::FitsHandler;

    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

