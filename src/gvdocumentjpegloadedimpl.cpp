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
#include <qcstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <ktempfile.h>

// Local
#include "gvimageutils/jpegcontent.h"
#include "gvimageutils/gvimageutils.h"
#include "gvdocumentjpegloadedimpl.moc"


//#define DEBUG_COMMENT

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

const char* JPEG_EXIF_DATA="Jpeg EXIF Data";
const char* JPEG_EXIF_COMMENT="Comment";


class GVDocumentJPEGLoadedImplPrivate {
public:
	GVImageUtils::JPEGContent mJPEGContent;
	QString mComment;
	GVDocument::CommentState mCommentState;
	QString mLocalFilePath;


	void loadComment() {
		KFileMetaInfo metaInfo=KFileMetaInfo(mLocalFilePath);
		KFileMetaInfoItem commentItem;

		mCommentState=GVDocument::NONE;

		if (metaInfo.isEmpty()) return;

		commentItem=metaInfo[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
		mCommentState=QFileInfo(mLocalFilePath).isWritable()?GVDocument::WRITABLE:GVDocument::READ_ONLY;
		mComment=QString::fromUtf8( commentItem.string().ascii() );

#ifdef DEBUG_COMMENT 
		// Some code to debug
		QStringList groups, keys;
		groups = metaInfo.groups();
		for (uint i=0; i<groups.size(); ++i) {
			keys = metaInfo[groups[i]].keys();
			kdDebug() << groups[i] << endl;
			for (uint j=0; j<keys.size(); ++j) {
				kdDebug() << "- " << keys[j] << endl;
			}
		}
#endif
	}

	void saveComment(const QString& path) {
		KFileMetaInfo metaInfo=KFileMetaInfo(path);
		KFileMetaInfoItem commentItem;
		if (metaInfo.isEmpty()) return;

		commentItem=metaInfo[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];

		if (commentItem.isEditable()) {
			commentItem.setValue(mComment);
		}
		metaInfo.applyChanges();
	}
};


/**
 * tempFilePath is set to QString::null if the document is a local file,
 * otherwise it is the name of a local copy of the file and must be deleted
 */
GVDocumentJPEGLoadedImpl::GVDocumentJPEGLoadedImpl(GVDocument* document, QByteArray& rawData, const QString& tempFilePath)
: GVDocumentLoadedImpl(document) {
	LOG("" << mDocument->url().prettyURL() << ", data size: " << rawData.size() );
	d=new GVDocumentJPEGLoadedImplPrivate;
	d->mJPEGContent.loadFromData(rawData);
	if (mDocument->url().isLocalFile()) {
		d->mLocalFilePath=document->url().path();
	} else {
		d->mLocalFilePath=tempFilePath;
	}
}


void GVDocumentJPEGLoadedImpl::init() {
	LOG("");
	GVImageUtils::Orientation orientation=d->mJPEGContent.orientation();

	if (orientation!=GVImageUtils::NOT_AVAILABLE && orientation!=GVImageUtils::NORMAL) {
		LOG("jpeg rotating");
		setImage(GVImageUtils::transform(mDocument->image(), orientation), true);
		d->mJPEGContent.transform(orientation);
	}

	d->loadComment();
	if (!mDocument->url().isLocalFile()) {
		QFile::remove(d->mLocalFilePath);
	}

	GVDocumentLoadedImpl::init();
}


GVDocumentJPEGLoadedImpl::~GVDocumentJPEGLoadedImpl() {
	delete d;
}


void GVDocumentJPEGLoadedImpl::transform(GVImageUtils::Orientation orientation) {
	d->mJPEGContent.transform(orientation);
	setImage(GVImageUtils::transform(mDocument->image(), orientation), true);
}


QString GVDocumentJPEGLoadedImpl::localSave(QFile* file, const QCString& format) const {
	if (qstrcmp(format, "JPEG")==0) {
		LOG("JPEG Reset orientation");
		d->mJPEGContent.resetOrientation();
		if (!d->mJPEGContent.thumbnail().isNull()) {
			d->mJPEGContent.setThumbnail( GVImageUtils::scale(
				mDocument->image(), 128, 128, GVImageUtils::SMOOTH_FAST, QImage::ScaleMin));
		}
		
		LOG("JPEG Lossless save");
		if (!d->mJPEGContent.save(file)) {
			return i18n("Could not save this JPEG file.");
		}
	} else {
		QString msg=GVDocumentLoadedImpl::localSave(file, format);
		if (!msg.isNull()) return msg;
	}

	d->saveComment(file->name());
	
	return QString::null;
}


QString GVDocumentJPEGLoadedImpl::comment() const {
	return d->mComment;
}

void GVDocumentJPEGLoadedImpl::setComment(const QString& comment) {
	d->mComment=comment;
}

GVDocument::CommentState GVDocumentJPEGLoadedImpl::commentState() const {
	return d->mCommentState;
}

