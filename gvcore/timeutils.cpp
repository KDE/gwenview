// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2006 Aurélien Gâteau

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
#include "timeutils.h"

// KDE
#include <kdebug.h>
#include <kfileitem.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <klocale.h>
	
namespace Gwenview {
namespace TimeUtils {

time_t getTime(const KFileItem* item) {
	const KFileMetaInfo& info = item->metaInfo();
	if (info.isValid()) {
		QVariant value = info.value("Date/time");
		QDateTime dt = value.toDateTime();
		if (dt.isValid()) {
			return dt.toTime_t();
		}
	}
	return item->time(KIO::UDS_MODIFICATION_TIME);
}

QString formatTime(time_t time) {
	QDateTime dt;
	dt.setTime_t(time);
	return KGlobal::locale()->formatDateTime(dt);
}


} // namespace TimeUtils
} // namespace Gwenview
