// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include "documentloadedimpl.moc"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qtimer.h>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <kio/netaccess.h>
#include <klargefile.h>
#include <klocale.h>
#include <ktempfile.h>

// Local
#include "imageutils/imageutils.h"
namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


class DocumentLoadedImplPrivate {
	int mSize;
	QDateTime mModified;
};

DocumentLoadedImpl::DocumentLoadedImpl(Document* document)
: DocumentImpl(document) {
	LOG("");
}


void DocumentLoadedImpl::init() {
	emit finished(true);
}


DocumentLoadedImpl::~DocumentLoadedImpl() {
}


void DocumentLoadedImpl::transform(ImageUtils::Orientation orientation) {
	setImage(ImageUtils::transform(mDocument->image(), orientation));
	emitImageRectUpdated();
}


QString DocumentLoadedImpl::save(const KURL& _url, const QCString& format) const {
	if (!QImageIO::outputFormats().contains(format)) {
		return i18n("Gwenview cannot write files in this format.");
	}

	QString msg;
	KURL url(_url);
	
	// Use the umask to determine default mode (will be used if the dest file
	// does not exist)
	int _umask=umask(0);
	umask(_umask);
	mode_t mode=0666 & ~_umask;

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
					return i18n("This is a circular link.");
				}
				info.setFile(path);
			}
			url.setPath(info.filePath());
		}

		
		// Make some quick tests on the file if it is local
		if (info.exists() && ! info.isWritable()) {
			return i18n("This file is read-only.");
		}
		
		if (info.exists()) {
			// Get current file mode
			KDE_struct_stat st; 
			if (KDE_stat(QFile::encodeName(info.filePath()), &st)==0) {
				mode=st.st_mode & 07777;
			} else {
				// This should not happen
				kdWarning() << "Could not stat " << info.filePath() << endl;
			}
			
		} else {
			QFileInfo parent=QFileInfo(info.dirPath());
			if (!parent.isWritable()) {
				return
					i18n("The %1 folder is read-only.")
					.arg(parent.filePath());
			}
		}
	}

	// Save the file to a tmp file
	QString prefix;
	if (url.isLocalFile()) {
		// We set the prefix to url.path() so that the temp file is on the
		// same partition as the destination file. If we don't do this, rename
		// will fail
		prefix=url.path();
	}
	KTempFile tmp(prefix, "gwenview", mode);
	tmp.setAutoDelete(true);
	if (tmp.status()!=0) {
		QString reason( strerror(tmp.status()) );
		return i18n("Could not create a temporary file.\nReason: %1.")
			.arg(reason);
	}
	QFile* file=tmp.file();
	msg=localSave(file, format);
	if (!msg.isNull()) return msg;
	file->close();
	
	if (tmp.status()!=0) {
		QString reason( strerror(tmp.status()) );
		return i18n("Saving image to a temporary file failed.\nReason: %1.")
			.arg(reason);
	}
	
	QString tmpName=tmp.name();
	int tmpSize=QFileInfo(tmpName).size();
	setFileSize(tmpSize);

	// Move the tmp file to the final dest
	if (url.isLocalFile()) {
		if( ::rename( QFile::encodeName(tmpName), QFile::encodeName( url.path())) < 0 ) {
			return i18n("Could not write to %1.").arg(url.path());
		}
	} else {
		if (!KIO::NetAccess::upload(tmp.name(), url, KApplication::kApplication()->mainWidget() )) {
			return i18n("Could not upload the file to %1.").arg(url.prettyURL());
		}
	}

	return QString::null;
}


QString DocumentLoadedImpl::localSave(QFile* file, const QCString& format) const {
	QImageIO iio(file, format);
	iio.setImage(mDocument->image());
	if (!iio.write()) {
		return
			i18n("An error happened while saving.");
	}
	return QString::null;
}


} // namespace
