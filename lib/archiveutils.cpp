// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2007 Aurélien Gâteau

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

// KDE
#include <kfileitem.h>
#include <kdebug.h>
// Local
#include "archiveutils.h"
namespace Gwenview {


namespace ArchiveUtils {

typedef QMap<QString,QString> ArchiveProtocolForMimeTypes;

static const ArchiveProtocolForMimeTypes& archiveProtocolForMimeTypes() {
	static ArchiveProtocolForMimeTypes map;
	static bool initialized = false;
	if (!initialized) {
		// FIXME: This code used to work in KDE3. For now stick with an
		// hardcoded list of mimetypes, stolen from Dolphin code
		#if 0
		static const char* KDE_PROTOCOL = "X-KDE-LocalProtocol";
		KMimeType::List list = KMimeType::allMimeTypes();
		KMimeType::List::Iterator it=list.begin(), end=list.end();
		for (; it!=end; ++it) {
			if ( (*it)->propertyNames().indexOf(KDE_PROTOCOL)!= -1 ) {
				QString protocol = (*it)->property(KDE_PROTOCOL).toString();
				map[(*it)->name()] = protocol;
			}
		}
		#endif
		map["application/zip"] = "zip";
		map["application/x-tar"] = "tar";
		map["application/x-tarz"] = "tar";
		map["application/x-bzip-compressed-tar"] = "tar";
		map["application/x-compressed-tar"] = "tar";
		map["application/x-tzo"] = "tar";
		initialized = true;
		if (map.empty()) {
			kWarning() << "No archive protocol found.";
		}
	}
	return map;
}

bool fileItemIsArchive(const KFileItem& item) {
	return archiveProtocolForMimeTypes().contains(item.mimetype());
}

bool fileItemIsDirOrArchive(const KFileItem& item) {
	return item.isDir() || fileItemIsArchive(item);
}

/*
bool protocolIsArchive(const QString& protocol) {
	const ArchiveProtocolForMimeTypes& map=archiveProtocolForMimeTypes();
	ArchiveProtocolForMimeTypes::ConstIterator it;
	for (it=map.begin();it!=map.end();++it) {
		if (it.data()==protocol) return true;
	}
	return false;
}
*/
QStringList mimeTypes() {
	return archiveProtocolForMimeTypes().keys();
}

QString protocolForMimeType(const QString& mimeType) {
	return archiveProtocolForMimeTypes()[mimeType];
}

} // namespace ArchiveUtils

} // namespace Gwenview
