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
// Self
#include "documentotherloadedimpl.h"

// KDE
#include <kdebug.h>
#include <kfilemetainfo.h>

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

namespace Gwenview {

int DocumentOtherLoadedImpl::duration() const {
	KFileMetaInfo fmi(mDocument->url());
	if (!fmi.isValid()) {
		LOG("No meta info available for " << mDocument->url());
		return 0;
	}
	
	KFileMetaInfoItem item=fmi.item("Length");
	if (!item.isValid()) {
		kdWarning() << "Can't adjust slideshow time: meta info for " << mDocument->url() << " does not contain 'Length' information.";
		return 0;
	}
	
	int length = item.value().toInt();
	LOG("Length for " << mDocument->url() << " is " << length);

	return length;
}

} // namespace
