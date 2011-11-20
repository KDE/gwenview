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
#define QT_STATICPLUGIN

// Self
#include "jpegplugin.h"

// Qt
#include <QImageIOHandler>

// Local
#include "jpeghandler.h"

namespace Gwenview
{

QStringList JpegPlugin::keys() const
{
    return QStringList() << QLatin1String("jpeg") << QLatin1String("jpg");
}

QImageIOPlugin::Capabilities JpegPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "jpeg" || format == "jpg") {
        return Capabilities(CanRead | CanWrite);
    }
    if (!format.isEmpty()) {
        return 0;
    }
    if (!device->isOpen()) {
        return 0;
    }

    Capabilities cap;
    if (device->isReadable() && JpegHandler::canRead(device)) {
        cap |= CanRead;
    }
    if (device->isWritable()) {
        cap |= CanWrite;
    }
    return cap;
}

QImageIOHandler *JpegPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new JpegHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

Q_EXPORT_STATIC_PLUGIN(JpegPlugin)

} // namespace
