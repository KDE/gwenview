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
#include <ktempfile.h>

// Local
#include "gvjpegtran.h"
#include "gvimageutils.h"
#include "gvdocumentjpegloadedimpl.moc"


//#define DEBUG_COMMENT


const char* JPEG_EXIF_DATA="Jpeg EXIF Data";
const char* JPEG_EXIF_COMMENT="Comment";


class GVDocumentJPEGLoadedImplPrivate {
public:
	QByteArray mRawData;
	QString mComment;
	GVDocument::CommentState mCommentState;
	QString mTempFilePath;


	void loadComment() {
		KFileMetaInfo metaInfo=KFileMetaInfo(mTempFilePath);
		KFileMetaInfoItem commentItem;

		mCommentState=GVDocument::NONE;

		if (metaInfo.isEmpty()) return;

		commentItem=metaInfo[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
		mCommentState=QFileInfo(mTempFilePath).isWritable()?GVDocument::WRITABLE:GVDocument::READ_ONLY;
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


GVDocumentJPEGLoadedImpl::GVDocumentJPEGLoadedImpl(GVDocument* document, QByteArray& rawData, const QString& tempFilePath)
: GVDocumentImpl(document) {
	kdDebug() << k_funcinfo << endl;
	d=new GVDocumentJPEGLoadedImplPrivate;
	d->mRawData=rawData;
	d->mTempFilePath=tempFilePath;

	QTimer::singleShot(0, this, SLOT(finishLoading()) );
}

GVDocumentJPEGLoadedImpl::~GVDocumentJPEGLoadedImpl() {
	delete d;
}


void GVDocumentJPEGLoadedImpl::modify(GVImageUtils::Orientation orientation) {
	if (!d->mRawData.isNull()) {
		d->mRawData=GVJPEGTran::apply(d->mRawData, orientation);
	}

	setImage(GVImageUtils::modify(mDocument->image(), orientation));
}


bool GVDocumentJPEGLoadedImpl::save(const KURL& url, const char* format) const {
	bool result;

	KTempFile tmp;
	tmp.setAutoDelete(true);
	QString path;
	if (url.isLocalFile()) {
		path=url.path();
	} else {
		path=tmp.name();
	}

	if (!d->mRawData.isNull() && qstrcmp(format, "JPEG")==0) {
		kdDebug() << "Lossless save\n";
		QFile file(path);
		result=file.open(IO_WriteOnly);
		if (!result) return false;
		QDataStream stream(&file);
		stream.writeRawBytes(d->mRawData.data(), d->mRawData.size());
		file.close();
	} else {
		result=mDocument->image().save(path, format);
		if (!result) return false;
	}

	d->saveComment(path);

	if (!url.isLocalFile()) {
		result=KIO::NetAccess::upload(tmp.name(), url);
	}

	return result;
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

void GVDocumentJPEGLoadedImpl::finishLoading() {
	kdDebug() << k_funcinfo << endl;
	GVImageUtils::Orientation orientation=GVImageUtils::getOrientation(d->mRawData);

	if (orientation!=GVImageUtils::NOT_AVAILABLE && orientation!=GVImageUtils::NORMAL) {
		kdDebug() << k_funcinfo << " jpeg rotating" << endl;
		setImage(GVImageUtils::modify(mDocument->image(), orientation));
		d->mRawData=GVJPEGTran::apply(d->mRawData, orientation);
		d->mRawData=GVImageUtils::setOrientation(d->mRawData,GVImageUtils::NORMAL);

		emit sizeUpdated(mDocument->image().width(), mDocument->image().height());
		emit rectUpdated(QRect(QPoint(0,0), mDocument->image().size()) );
	}

	d->loadComment();
	KIO::NetAccess::removeTempFile(d->mTempFilePath);

	emit finished(true);
}
