// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2026 Robert Siebeck <robert@siebeck.org>

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
// Self
#include "motionphotoutil.h"

// Qt
#include <QFile>
#include <QString>

// Exiv2
#include <exiv2/exiv2.hpp>

// Local
#include "gwenview_lib_debug.h"

namespace Gwenview
{
namespace MotionPhotoUtil
{

static void ensureNamespacesRegistered()
{
    static bool registered = [] {
        Exiv2::XmpParser::initialize();
        Exiv2::XmpProperties::registerNs("http://ns.google.com/photos/1.0/camera/", "GCamera");
        return true;
    }();
    (void)registered;
}

bool isMotionPhoto(const QString &path)
{
    ensureNamespacesRegistered();

    try {
        QByteArray filePathBytes = QFile::encodeName(path);
        auto image = Exiv2::ImageFactory::open(filePathBytes.constData());
        if (!image.get() || !image->good()) {
            return false;
        }
        image->readMetadata();

        auto &xmpData = image->xmpData();

        auto it = xmpData.findKey(Exiv2::XmpKey("Xmp.GCamera.MotionPhoto"));
        if (it != xmpData.end() && it->toString() == "1") {
            return true;
        }
    } catch (...) {
    }

    return false;
}

QByteArray extractVideo(const QString &path)
{
    ensureNamespacesRegistered();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(GWENVIEW_LIB_LOG) << "Failed to open motion photo file:" << path;
        return {};
    }
    const QByteArray fileData = file.readAll();

    // Motion photos embed the MP4 after the JPEG data. The MP4 starts with a
    // ftyp box: [4 bytes big-endian size][4 bytes "ftyp"][...].
    // We scan forward from a safe offset past the JPEG header.
    const int minOffset = 4096;
    for (int i = minOffset; i < fileData.size() - 8; ++i) {
        if (fileData[i + 4] == 'f' && fileData[i + 5] == 't' && fileData[i + 6] == 'y' && fileData[i + 7] == 'p') {
            const quint32 boxSize = (quint8(fileData[i]) << 24) | (quint8(fileData[i + 1]) << 16) | (quint8(fileData[i + 2]) << 8) | quint8(fileData[i + 3]);
            if (boxSize >= 8 && static_cast<qint64>(boxSize) <= fileData.size() - i) {
                qCDebug(GWENVIEW_LIB_LOG) << "Extracted motion photo video via ftyp scan at offset" << i << "from" << path;
                return fileData.mid(i);
            }
        }
    }

    qCWarning(GWENVIEW_LIB_LOG) << "Could not extract embedded video from motion photo:" << path;
    return {};
}

} // namespace MotionPhotoUtil
} // namespace Gwenview
