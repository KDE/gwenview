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
#include "mimetypeutils.h"

// Qt
#include <qstringlist.h>

// KDE
#include <kfileitem.h>
/* FIXME
#include <kapplication.h>
#include <kio/netaccess.h>
#include <kmimetype.h>
#include <kurl.h>
*/

#include <kimageio.h>

// Local
#include "archiveutils.h"


namespace Gwenview {

namespace MimeTypeUtils {


const QStringList& dirMimeTypes() {
	static QStringList list;
	if (list.isEmpty()) {
		list << "inode/directory";
		list += ArchiveUtils::mimeTypes();
	}
	return list;
}


const QStringList& rasterImageMimeTypes() {
	static QStringList list;
	if (list.isEmpty()) {
		list=KImageIO::mimeTypes(KImageIO::Reading);
		list.append("image/x-xcf");
		list.append("image/x-xcursor");
	}
	return list;
}


const QStringList& imageMimeTypes() {
	static QStringList list;
	if (list.isEmpty()) {
		list = rasterImageMimeTypes();
		list.append("image/svg+xml");
	}

	return list;
}


const QStringList& videoMimeTypes() {
	static QStringList list;
	if (list.isEmpty()) {
#ifdef __GNUC__
        #warning implement MimeTypeUtils::videoMimeTypes()
#endif
	}

	return list;
}


Kind mimeTypeKind(const QString& mimeType) {
	if (mimeType.startsWith("inode/directory")) {
		return KIND_DIR;
	}
	if (ArchiveUtils::mimeTypes().contains(mimeType)) {
		return KIND_ARCHIVE;
	}
	if (rasterImageMimeTypes().contains(mimeType)) {
		return KIND_RASTER_IMAGE;
	}

	return KIND_FILE;
}

	
Kind fileItemKind(const KFileItem* item) {
	return mimeTypeKind(item->mimetype());
}


/*
Kind urlKind(const KURL& url) {
	QString mimeType;
	if (url.isLocalFile()) {
		mimeType=KMimeType::findByURL(url)->name();
	} else {
		mimeType=KIO::NetAccess::mimetype(url, KApplication::kApplication()->mainWidget());
	}
	return mimeTypeKind(mimeType);
}
*/

} // namespace MimeTypeUtils
} // namespace Gwenview
