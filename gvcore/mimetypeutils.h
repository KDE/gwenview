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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef MIMETYPEUTILS_H
#define MIMETYPEUTILS_H   

// Qt
#include <qcstring.h>

// Local
#include "libgwenview_export.h"

class KFileItem;
class KURL;

class QStringList;

namespace Gwenview {

namespace MimeTypeUtils {
enum Kind { KIND_UNKNOWN, KIND_DIR, KIND_ARCHIVE, KIND_FILE, KIND_RASTER_IMAGE };

LIBGWENVIEW_EXPORT const QStringList& rasterImageMimeTypes();
Kind fileItemKind(const KFileItem*);
Kind urlKind(const KURL&);
Kind determineKindFromContent(const QByteArray&);

} // namespace FileUtils

} // namespace Gwenview

#endif /* MIMETYPEUTILS_H */
