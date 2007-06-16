// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurelien Gateau

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
#ifndef MIMETYPEUTILS_H
#define MIMETYPEUTILS_H

#include "gwenviewlib_export.h"

// Local
class QStringList;

class KFileItem;

namespace Gwenview {

namespace MimeTypeUtils {

GWENVIEWLIB_EXPORT const QStringList& dirMimeTypes();
GWENVIEWLIB_EXPORT const QStringList& imageMimeTypes();
GWENVIEWLIB_EXPORT const QStringList& videoMimeTypes();

enum Kind { KIND_UNKNOWN, KIND_DIR, KIND_ARCHIVE, KIND_FILE, KIND_RASTER_IMAGE };
GWENVIEWLIB_EXPORT Kind fileItemKind(const KFileItem*);
//Kind urlKind(const KURL&);
GWENVIEWLIB_EXPORT Kind mimeTypeKind(const QString& mimeType);

} // namespace MimeTypeUtils

} // namespace Gwenview

#endif /* MIMETYPEUTILS_H */
