// vim: set tabstop=4 shiftwidth=4 expandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau <agateau@kde.org>

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
#ifndef ARCHIVEUTILS_H
#define ARCHIVEUTILS_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QString>
class KFileItem;

namespace Gwenview
{
/**
 * Helper functions to deal with archives
 */
namespace ArchiveUtils
{
/**
 * Returns true if @p item is an archive
 */
GWENVIEWLIB_EXPORT bool fileItemIsArchive(const KFileItem &item);

/**
 * Returns true if @p item is a dir or an archive
 */
GWENVIEWLIB_EXPORT bool fileItemIsDirOrArchive(const KFileItem &item);

/**
 * Returns the protocol for an archive mime type. Similar to
 * KProtocolManager::protocolForArchiveMimetype(), except it tries parent
 * mimetypes if it can't find anything (useful for .cbz and co)
 */
GWENVIEWLIB_EXPORT QString protocolForMimeType(const QString &mimeType);

} // namespace ArchiveUtils

} // namespace Gwenview
#endif
