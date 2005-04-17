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
#include <kio/netaccess.h>
#include <klocale.h>

// Local
#include "gvimageutils/jpegcontent.h"
#include "gvimageutils/gvimageutils.h"
#include "gvdocumentjpegloadedimpl.moc"


//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


class GVDocumentJPEGLoadedImplPrivate {
public:
	GVImageUtils::JPEGContent mJPEGContent;
	bool mCommentModified;
	QString mComment;
	GVDocument::CommentState mCommentState;

};


GVDocumentJPEGLoadedImpl::GVDocumentJPEGLoadedImpl(GVDocument* document, const QByteArray& rawData)
: GVDocumentLoadedImpl(document) {
	LOG("" << mDocument->url().prettyURL() << ", data size: " << rawData.size() );
	d=new GVDocumentJPEGLoadedImplPrivate;
	d->mCommentModified=false;
	d->mJPEGContent.loadFromData(rawData);
}


void GVDocumentJPEGLoadedImpl::init() {
	LOG("");
	GVImageUtils::Orientation orientation=d->mJPEGContent.orientation();

	if (orientation!=GVImageUtils::NOT_AVAILABLE && orientation!=GVImageUtils::NORMAL) {
		LOG("jpeg rotating");
		setImage(GVImageUtils::transform(mDocument->image(), orientation), true);
		d->mJPEGContent.transform(orientation);
	}

	d->mCommentState=GVDocument::WRITABLE;
	d->mComment=d->mJPEGContent.comment();
	GVDocumentLoadedImpl::init();
}


GVDocumentJPEGLoadedImpl::~GVDocumentJPEGLoadedImpl() {
	delete d;
}


void GVDocumentJPEGLoadedImpl::transform(GVImageUtils::Orientation orientation) {
	// Little optimization, update the comment if necessary
	d->mJPEGContent.transform(orientation, d->mCommentModified, d->mComment);
	d->mCommentModified=false;
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

		if (d->mCommentModified) {
			LOG("Comment must be saved");
			d->mJPEGContent.transform(GVImageUtils::NORMAL, true, d->mComment);
			d->mCommentModified=false;
		}
		
		LOG("JPEG Lossless save");
		if (!d->mJPEGContent.save(file)) {
			return i18n("Could not save this JPEG file.");
		}
	} else {
		QString msg=GVDocumentLoadedImpl::localSave(file, format);
		if (!msg.isNull()) return msg;
	}
	
	return QString::null;
}


QString GVDocumentJPEGLoadedImpl::comment() const {
	return d->mComment;
}

void GVDocumentJPEGLoadedImpl::setComment(const QString& comment) {
	d->mCommentModified=true;
	d->mComment=comment;
}

GVDocument::CommentState GVDocumentJPEGLoadedImpl::commentState() const {
	return d->mCommentState;
}

