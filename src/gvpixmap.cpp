/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau
 
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

#include <sys/stat.h> // For S_ISDIR

// Qt includes
#include <qfileinfo.h>
#include <qpainter.h>
#include <qwmatrix.h>

// KDE includes
#include <kdebug.h>
#include <kio/netaccess.h>

// Our includes
#include <gvarchive.h>
#include <gvpixmap.moc>


GVPixmap::GVPixmap(QObject* parent) 
: QObject(parent) 
{}


GVPixmap::~GVPixmap()
{}


void GVPixmap::setURL(const KURL& paramURL) {
	//kdDebug() << "GVPixmap::setURL " << paramURL.prettyURL() << endl;
	KURL URL(paramURL);
	if (URL.cmp(url())) return;

	// Fix wrong protocol
	if (GVArchive::protocolIsArchive(URL.protocol())) {
		QFileInfo info(URL.path());
		if (info.exists()) {
			URL.setProtocol("file");
		}
	}

	// Check whether this is a dir or a file
	KIO::UDSEntry entry;
	bool isDir=false;
	if (!KIO::NetAccess::stat(URL,entry)) return;
	KIO::UDSEntry::ConstIterator it;
	for(it=entry.begin();it!=entry.end();++it) {
		if ((*it).m_uds==KIO::UDS_FILE_TYPE) {
			isDir=S_ISDIR( (*it).m_long );
			break;
		}
	}

	if (isDir) {
		mDirURL=URL;
		mFilename="";
	} else {
		mDirURL=URL.upURL();
		mFilename=URL.filename();
	}
	
	if (mFilename.isEmpty()) {
		reset();
		return;
	}

	emit loading();
	if (!load()) {
		reset();
		return;
	}
	emit urlChanged(mDirURL,mFilename);
}


void GVPixmap::setDirURL(const KURL& paramURL) {
	mDirURL=paramURL;
	mFilename="";
	reset();
}


void GVPixmap::setFilename(const QString& filename) {
	if (mFilename==filename) return;
	
	mFilename=filename;
	emit loading();
	if (!load()) {
		reset();
		return;
	}
	emit urlChanged(mDirURL,mFilename);
}


void GVPixmap::reset() {
	mImage.reset();
	emit urlChanged(mDirURL,mFilename);
}


KURL GVPixmap::url() const {
	KURL url=mDirURL;
	url.addPath(mFilename);
	return url;
}


void GVPixmap::rotateLeft() {
	QWMatrix matrix;
	matrix.rotate(-90);
	mImage=mImage.xForm(matrix);
	emit modified();
}


void GVPixmap::rotateRight() {
	QWMatrix matrix;
	matrix.rotate(90);
	mImage=mImage.xForm(matrix);
	emit modified();
}


void GVPixmap::mirror() {
	QWMatrix matrix;
	matrix.scale(-1,1);
	mImage=mImage.xForm(matrix);
	emit modified();
}


void GVPixmap::flip() {
	QWMatrix matrix;
	matrix.scale(1,-1);
	mImage=mImage.xForm(matrix);
	emit modified();
}


//---------------------------------------------------------------------
//
// Private stuff
// 
//---------------------------------------------------------------------
bool GVPixmap::load() {
	KURL pixURL=url();
	bool result;
	//kdDebug() << "GVPixmap::load() " << pixURL.prettyURL() << endl;
		
	// FIXME : Async
	QString path;
	if (pixURL.isLocalFile()) {
		path=pixURL.path();
	} else {
		if (!KIO::NetAccess::download(pixURL,path)) return false;
	}
	result=mImage.load(path);
	
	if (!pixURL.isLocalFile()) {
		KIO::NetAccess::removeTempFile(path);
	}

	return result;
}
