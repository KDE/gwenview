/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau
 
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
#include "gvarchive.h"


namespace GVArchive {

bool fileItemIsArchive(const KFileItem* item) {
	return mimeTypes().findIndex(item->mimetype())!=-1;
}

bool protocolIsArchive(const QString& protocol) {
	return protocols().findIndex(protocol)!=-1;
}

const QStringList& mimeTypes() {
	static QStringList sMimeTypes;
	if (sMimeTypes.empty()) {
		sMimeTypes.append("application/x-tar");
		sMimeTypes.append("application/x-tgz");
		sMimeTypes.append("application/x-tbz");
	}
	return sMimeTypes;
}

const QStringList& protocols() {
	static QStringList sProtocols;
	if (sProtocols.empty()) {
		sProtocols.append("tar");
	}
	return sProtocols;
}

QString protocolForMimeType(const QString& mimeType) {
	if (mimeTypes().findIndex(mimeType)) {
		return "tar";
	} else {
		return QString::null;
	}
}

}
