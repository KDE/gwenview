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
#include "miscconfig.h"
#include "imageutils/jpegcontent.h"
#include "imageutils/imageutils.h"
#include "documentjpegloadedimpl.moc"
namespace Gwenview {


#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


class DocumentJPEGLoadedImplPrivate {
public:
	ImageUtils::JPEGContent mJPEGContent;

};


DocumentJPEGLoadedImpl::DocumentJPEGLoadedImpl(Document* document, const QByteArray& rawData)
: DocumentLoadedImpl(document) {
	LOG("" << mDocument->url().prettyURL() << ", data size: " << rawData.size() );
	d=new DocumentJPEGLoadedImplPrivate;
	d->mJPEGContent.loadFromData(rawData);
}


void DocumentJPEGLoadedImpl::init() {
	LOG("");
	ImageUtils::Orientation orientation=d->mJPEGContent.orientation();

	if (MiscConfig::autoRotateImages()
		&& orientation!=ImageUtils::NOT_AVAILABLE
		&& orientation!=ImageUtils::NORMAL)
	{
		LOG("jpeg rotating");
		setImage(ImageUtils::transform(mDocument->image(), orientation), true);
		d->mJPEGContent.transform(orientation);
	}

	DocumentLoadedImpl::init();
}


DocumentJPEGLoadedImpl::~DocumentJPEGLoadedImpl() {
	delete d;
}


void DocumentJPEGLoadedImpl::transform(ImageUtils::Orientation orientation) {
	d->mJPEGContent.transform(orientation);
	setImage(ImageUtils::transform(mDocument->image(), orientation), true);
}


QString DocumentJPEGLoadedImpl::localSave(QFile* file, const QCString& format) const {
	if (qstrcmp(format, "JPEG")==0) {
		LOG("JPEG Reset orientation");
		d->mJPEGContent.resetOrientation();
		if (!d->mJPEGContent.thumbnail().isNull()) {
			d->mJPEGContent.setThumbnail( ImageUtils::scale(
				mDocument->image(), 128, 128, ImageUtils::SMOOTH_FAST, QImage::ScaleMin));
		}

		LOG("JPEG Lossless save");
		if (!d->mJPEGContent.save(file)) {
			return i18n("Could not save this JPEG file.");
		}
	} else {
		QString msg=DocumentLoadedImpl::localSave(file, format);
		if (!msg.isNull()) return msg;
	}
	
	return QString::null;
}


QString DocumentJPEGLoadedImpl::comment() const {
	return d->mJPEGContent.comment();
}

void DocumentJPEGLoadedImpl::setComment(const QString& comment) {
	d->mJPEGContent.setComment(comment);
}

Document::CommentState DocumentJPEGLoadedImpl::commentState() const {
	return Document::WRITABLE;
}


} // namespace
