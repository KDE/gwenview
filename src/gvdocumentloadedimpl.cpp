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
#include <qfileinfo.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <kio/netaccess.h>
#include <ktempfile.h>

// Local
#include "gvimageutils/gvimageutils.h"
#include "gvdocumentloadedimpl.moc"

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


class GVDocumentLoadedImplPrivate {
	int mSize;
	QDateTime mModified;
};

GVDocumentLoadedImpl::GVDocumentLoadedImpl(GVDocument* document)
: GVDocumentImpl(document) {
	LOG("");
	
	// Do not emit the signal directly from the constructor because
	// switchToImpl has not updated the connections yet
	QTimer::singleShot(0, this, SLOT(finishLoading()) );
}


GVDocumentLoadedImpl::~GVDocumentLoadedImpl() {
}


void GVDocumentLoadedImpl::transform(GVImageUtils::Orientation orientation) {
	setImage(GVImageUtils::transform(mDocument->image(), orientation));
}


bool GVDocumentLoadedImpl::save(const KURL& url, const QCString& format) const {
	bool result;

	KTempFile tmp;
	tmp.setAutoDelete(true);
	QString path;
	if (url.isLocalFile()) {
		path=url.path();
	} else {
		path=tmp.name();
	}

	result=localSave(path, format);
	if (!result) return false;
	setFileSize(QFileInfo(path).size());

	if (!url.isLocalFile()) {
		result=KIO::NetAccess::upload(tmp.name(),url);
	}

	return result;
}


bool GVDocumentLoadedImpl::localSave(const QString& path, const QCString& format) const {
	return mDocument->image().save(path, format);
}

void GVDocumentLoadedImpl::finishLoading() {
	emit finished(true);
}
