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
// Qt
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <kio/netaccess.h>
#include <ktempfile.h>

// Local
#include "gvdocumentloadedimpl.moc"



GVDocumentLoadedImpl::GVDocumentLoadedImpl(GVDocument* document)
: GVDocumentImpl(document) {
	kdDebug() << k_funcinfo << endl;
	
	// Do not emit the signal directly from the constructor because
	// switchToImpl has not updated the connections yet
	QTimer::singleShot(0, this, SLOT(emitFinished()) );
}


GVDocumentLoadedImpl::~GVDocumentLoadedImpl() {
}


void GVDocumentLoadedImpl::modify(GVImageUtils::Orientation orientation) {
	setImage(GVImageUtils::modify(mDocument->image(), orientation));
}


bool GVDocumentLoadedImpl::save(const KURL& url, const char* format) const {
	bool result;

	KTempFile tmp;
	tmp.setAutoDelete(true);
	QString path;
	if (url.isLocalFile()) {
		path=url.path();
	} else {
		path=tmp.name();
	}

	result=mDocument->image().save(path, format);
	if (!result) return false;

	if (!url.isLocalFile()) {
		result=KIO::NetAccess::upload(tmp.name(),url);
	}

	return result;
}
	

void GVDocumentLoadedImpl::emitFinished() {
	emit finished(true);
}
