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

// Local
#include "document/document.h"
#include "gwenview_lib_debug.h"
#include "imagemetainfomodel.h"

namespace Gwenview
{
namespace MotionPhotoUtil
{

bool isMotionPhoto(const Document &document)
{
    return document.metaInfo()->getValueForKey(QStringLiteral("Xmp.GCamera.MotionPhoto")) == QLatin1String("1");
}

QByteArray extractVideo(const QByteArray &fileData)
{
    // Motion photos embed the MP4 after the JPEG data. The MP4 starts with a
    // ftyp box: [4 bytes big-endian size][4 bytes "ftyp"][...].
    // We scan forward from a safe offset past the JPEG header.
    const qsizetype minOffset = 4096;
    const QByteArray ftypMarker("ftyp");
    qsizetype pos = minOffset + 4;
    while ((pos = fileData.indexOf(ftypMarker, pos)) != -1) {
        const qsizetype boxStart = pos - 4;
        const quint32 boxSize = (quint8(fileData[boxStart]) << 24) | (quint8(fileData[boxStart + 1]) << 16) | (quint8(fileData[boxStart + 2]) << 8)
            | quint8(fileData[boxStart + 3]);
        if (boxSize >= 8 && static_cast<qint64>(boxSize) <= fileData.size() - boxStart) {
            qCDebug(GWENVIEW_LIB_LOG) << "Extracted motion photo video via ftyp scan at offset" << boxStart;
            return fileData.mid(boxStart);
        }
        ++pos;
    }

    qCWarning(GWENVIEW_LIB_LOG) << "Could not extract embedded video from motion photo";
    return {};
}

} // namespace MotionPhotoUtil
} // namespace Gwenview
