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
#include <kimageio.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>

// Our includes
#include <gvarchive.h>
#include <gvimagesavedialog.h>
#include <gvjpegtran.h>

#include <gvpixmap.moc>

GVPixmap::GVPixmap(QObject* parent)
: QObject(parent)
, mModified(false)
{}


GVPixmap::~GVPixmap() {
}


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

	// Ask to save if necessary. Do this now because mDirURL and mFilename
	// have not been modified yet.
	saveIfModified();

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
	load();
	emit urlChanged(mDirURL,mFilename);
}


void GVPixmap::setDirURL(const KURL& paramURL) {
	mDirURL=paramURL;
	mFilename="";
	reset();
}


void GVPixmap::setFilename(const QString& filename) {
	if (mFilename==filename) return;

	saveIfModified();
	mFilename=filename;
	emit loading();
	load();
	emit urlChanged(mDirURL,mFilename);
}


KURL GVPixmap::url() const {
	KURL url=mDirURL;
	url.addPath(mFilename);
	return url;
}


void GVPixmap::rotateLeft() {
	// Apply the rotation to the compressed data too if available
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		kdDebug() << "Lossless left rotation\n";
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVJPEGTran::RotateLeft);
	}
	QWMatrix matrix;
	matrix.rotate(-90);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::rotateRight() {
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVJPEGTran::RotateRight);
	}
	QWMatrix matrix;
	matrix.rotate(90);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::mirror() {
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVJPEGTran::Mirror);
	}
	QWMatrix matrix;
	matrix.scale(-1,1);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::flip() {
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVJPEGTran::Flip);
	}
	QWMatrix matrix;
	matrix.scale(1,-1);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::save() {
	if (!saveInternal(url(),mImageFormat)) {
		KMessageBox::sorry(0,i18n("Could not save file."));
	}
}


void GVPixmap::saveAs() {
	QString format=mImageFormat;
	KURL saveURL;
	if (url().isLocalFile()) saveURL=url();

	GVImageSaveDialog dialog(saveURL,format,0);
	if (!dialog.exec()) return;

	if (!saveInternal(saveURL,format)) {
		KMessageBox::sorry(0,i18n("Could not save file."));
	}
}


//---------------------------------------------------------------------
//
// Private stuff
//
//---------------------------------------------------------------------
void GVPixmap::load() {
	KURL pixURL=url();
	//kdDebug() << "GVPixmap::load() " << pixURL.prettyURL() << endl;

	// FIXME : Async
	QString path;
	if (pixURL.isLocalFile()) {
		path=pixURL.path();
	} else {
		if (!KIO::NetAccess::download(pixURL,path)) {
			mImage.reset();
			return;
		}
	}

	// Load file. We load it ourself so that we can keep a copy of the
	// compressed data. This is used by jpeg loss-less manipulations.
	QFile file(path);
	file.open(IO_ReadOnly);
	QDataStream stream(&file);
	mCompressedData.resize(file.size());
	stream.readRawBytes(mCompressedData.data(),mCompressedData.size());

	// Decode image
	mImageFormat=QString(QImage::imageFormat(path));
	if (mImage.loadFromData(mCompressedData,mImageFormat.ascii())) {
		// Throw data away if it's not a JPEG, since we wont use it
		if (mImageFormat!="JPEG") mCompressedData.resize(0);
	} else {
		mImage.reset();
	}

	if (!pixURL.isLocalFile()) {
		KIO::NetAccess::removeTempFile(path);
	}
}


bool GVPixmap::saveInternal(const KURL& url, const QString& format) {
	bool result;

	KTempFile tmp;
	tmp.setAutoDelete(true);
	QString path;
	if (url.isLocalFile()) {
		path=url.path();
	} else {
		path=tmp.name();
	}
	
	if (format=="JPEG" && !mCompressedData.isNull()) {
		kdDebug() << "Lossless save\n";
		QFile file(path);
		result=file.open(IO_WriteOnly);
		if (!result) return false;
		QDataStream stream(&file);
		stream.writeRawBytes(mCompressedData.data(),mCompressedData.size());
	} else {
		result=mImage.save(path,format.ascii());
		if (!result) return false;
	}

	if (!url.isLocalFile()) {
		result=KIO::NetAccess::upload(tmp.name(),url);
	}
	
	if (result) {
		emit saved(url);
		mModified=false;
	}

	return result;
}


void GVPixmap::reset() {
	mImage.reset();
	mCompressedData.resize(0);
	emit urlChanged(mDirURL,mFilename);
}


void GVPixmap::saveIfModified() {
	if (!mModified) return;
	mModified=false;
	QString msg=i18n("The current image has been modified, do you want to save the changes?");
	if (KMessageBox::questionYesNo(0,msg)==KMessageBox::Yes) {
		saveAs();
	}
}

