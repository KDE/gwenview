// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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

// KDE includes
#include <kfileitem.h>

// Our includes
#include "archive.h"
namespace Gwenview {


namespace Archive {

typedef QMap<QString,QString> MimeTypeProtocols;

static const MimeTypeProtocols& mimeTypeProtocols() {
	static MimeTypeProtocols map;
	if (map.isEmpty()) {
		map["application/x-tar"]="tar";
		map["application/x-tgz"]="tar";
		map["application/x-tbz"]="tar";
		map["application/x-zip"]="zip";
	}
	return map;
}

	
bool fileItemIsArchive(const KFileItem* item) {
	return mimeTypeProtocols().contains(item->mimetype());
}

bool fileItemIsDirOrArchive(const KFileItem* item) {
	return item->isDir() || Archive::fileItemIsArchive(item);
}

bool protocolIsArchive(const QString& protocol) {
	const MimeTypeProtocols& map=mimeTypeProtocols();
	MimeTypeProtocols::ConstIterator it;
	for (it=map.begin();it!=map.end();++it) {
		if (it.data()==protocol) return true;
	}
	return false;
}

QStringList mimeTypes() {
	const MimeTypeProtocols& map=mimeTypeProtocols();
	MimeTypeProtocols::ConstIterator it;
	QStringList strlist;
	for (it=map.begin();it!=map.end();++it) {
		strlist+=it.key();
	}
	return strlist;
	//return mimeTypeProtocols().keys(); // keys() does not exist in Qt 3.0
}


QString protocolForMimeType(const QString& mimeType) {
	return mimeTypeProtocols()[mimeType];
}

}

} // namespace
