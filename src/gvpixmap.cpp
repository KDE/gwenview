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

#include <sys/stat.h> // For S_ISDIR

// Qt includes
#include <qfileinfo.h>
#include <qpainter.h>

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
	bool isDir;
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
	mPixmap.resize(0,0);
	emit urlChanged(mDirURL,mFilename);
}


KURL GVPixmap::url() const {
	KURL url=mDirURL;
	url.addPath(mFilename);
	return url;
}


//-Private-------------------------------------------------------------
bool GVPixmap::load() {
	KURL pixURL=url();
	kdDebug() << "GVPixmap::load() " << pixURL.prettyURL() << endl;
	int posX,posY;
	int pixWidth;
	int pixHeight;
	QPainter painter;
	QColor dark(128,128,128);
	QColor light(192,192,192);
	QPixmap pix;

	// Load pixmap
	// FIXME : Async
	QString path;
	if (pixURL.isLocalFile()) {
		path=pixURL.path();
	} else {
		if (!KIO::NetAccess::download(pixURL,path)) return false;
	}
	if (!pix.load(path)) return false;
	if (!pixURL.isLocalFile()) {
		KIO::NetAccess::removeTempFile(path);
	}
	
	pixWidth=pix.width();
	pixHeight=pix.height();

	// Create checker board
	mPixmap.resize(pixWidth,pixHeight);
	mPixmap.fill(dark);
	painter.begin(&mPixmap);
	for(posY=0;posY<pixHeight;posY+=16) {
		for(posX=0;posX<pixWidth;posX+=16) {
			painter.fillRect(posX,posY,8,8,light);
			painter.fillRect(posX+8,posY+8,8,8,light);
		}
	}

	// Paint pixmap on checker board 
	painter.drawPixmap(0,0,pix);
	painter.end();
	return true;
}
