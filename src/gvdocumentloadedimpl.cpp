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

#include <stdio.h>
#include <unistd.h>

// Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <kio/netaccess.h>
#include <klocale.h>
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
}


void GVDocumentLoadedImpl::init() {
	emit finished(true);
}


GVDocumentLoadedImpl::~GVDocumentLoadedImpl() {
}


void GVDocumentLoadedImpl::transform(GVImageUtils::Orientation orientation) {
	setImage(GVImageUtils::transform(mDocument->image(), orientation));
}


QString GVDocumentLoadedImpl::save(const KURL& _url, const QCString& format) const {
	QString msg;
	KURL url(_url);
	
	if (url.isLocalFile()) {
		// If the file is a link, dereference it but take care of circular
		// links
		QFileInfo info(url.path());
		if (info.isSymLink()) {
			QStringList links;
			while (info.isSymLink()) {
				links.append(info.filePath());
				QString path=info.readLink();
				if (path[0]!='/') {
					path=info.dirPath(true) + '/' + path;
				}
				path=QDir::cleanDirPath(path);
				if (links.contains(path)) {
					return i18n("<qt><b>Could not save the image to %1.</b><br/>This is a circular link.</qt>")
					.arg(url.path());
				}
				info.setFile(path);
			}
			url.setPath(info.filePath());
		}
		
		// Make some quick tests on the file if it is local
		if (info.exists() && ! info.isWritable()) {
			return
				i18n("<qt><b>Could not save the image to %1.</b><br/>This file is read-only.</qt>")
				.arg(info.filePath());
		}
		if (!info.exists()) {
			QFileInfo parent=QFileInfo(info.dirPath());
			if (!parent.isWritable()) {
				return
					i18n("<qt><b>Could not save the image to %1.</b><br/>The %2 folder is read-only.</qt>")
					.arg(info.filePath())
					.arg(parent.filePath());
			}
		}
	}

	// Save the file to a tmp file
	QString prefix;
	if (url.isLocalFile()) {
		// We set the prefix to url.path() so that the temp file is one the
		// same partition as the destination file. If we don't do this, rename
		// will fail
		prefix=url.path();
	}
	KTempFile tmp(prefix);
	tmp.setAutoDelete(true);

	msg=localSave(tmp.file(), format);
	if (!msg.isNull()) return msg;
	setFileSize(QFileInfo(tmp.name()).size());

	// Move the tmp file to the final dest
	if (url.isLocalFile()) {
		if( ::rename( QFile::encodeName( tmp.name() ), QFile::encodeName( url.path())) < 0 ) {
			return i18n("Could not save the image to %1").arg(url.path());
		}
	} else {
		if (!KIO::NetAccess::upload(tmp.name(),url)) {
			return i18n("Could not upload the file to %1").arg(url.prettyURL());
		}
	}

	return QString::null;
}


QString GVDocumentLoadedImpl::localSave(QFile* file, const QCString& format) const {
	QImageIO iio(file, format);
	iio.setImage(mDocument->image());
	if (!iio.write()) {
		return
			i18n("<qt><b>Could not save the image to %1.</b><br/>An error happened while saving.</qt>")
			.arg(file->name());
	}
	return QString::null;
}

