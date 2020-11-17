// vim: set tabstop=4 shiftwidth=4 expandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2007 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "archiveutils.h"

// Qt
#include <QMimeDatabase>

// KF
#include <KFileItem>
#include <KProtocolManager>

// Local
#include "gwenview_lib_debug.h"

namespace Gwenview
{

namespace ArchiveUtils
{

bool fileItemIsArchive(const KFileItem& item)
{
    QMimeType mimeType = item.determineMimeType();
    if (!mimeType.isValid()) {
        qCWarning(GWENVIEW_LIB_LOG) << "determineMimeType() returned a null pointer";
        return false;
    }
    return !ArchiveUtils::protocolForMimeType(mimeType.name()).isEmpty();
}

bool fileItemIsDirOrArchive(const KFileItem& item)
{
    return item.isDir() || fileItemIsArchive(item);
}

QString protocolForMimeType(const QString& mimeType)
{
    static QHash<QString, QString> cache;
    QHash<QString, QString>::ConstIterator it = cache.constFind(mimeType);
    if (it != cache.constEnd()) {
        return it.value();
    }

    if (mimeType == QLatin1String("image/svg+xml-compressed")) {
        // We don't want .svgz to be considered as archives because QtSvg knows
        // how to decode gzip-ed svg files
        cache.insert(mimeType, QString());
        return QString();
    }

    QString protocol = KProtocolManager::protocolForArchiveMimetype(mimeType);
    if (protocol.isEmpty()) {
        // No protocol, try with mimeType parents. This is useful for .cbz for
        // example
        const QMimeType mime = QMimeDatabase().mimeTypeForName(mimeType);
        const QStringList allAncestors = mime.allAncestors();
        for(const QString & parentMimeType : allAncestors) {
            protocol = KProtocolManager::protocolForArchiveMimetype(parentMimeType);
            if (!protocol.isEmpty()) {
                break;
            }
        }
    }

    cache.insert(mimeType, protocol);
    return protocol;
}

} // namespace ArchiveUtils

} // namespace Gwenview
