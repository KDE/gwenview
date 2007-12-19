// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "documentloadedimpl.h"

// Qt
#include <QImage>
#include <QMatrix>

// KDE
#include <kdebug.h>
#include <ksavefile.h>
#include <kurl.h>

// Local
#include "imageutils.h"

namespace Gwenview {


struct DocumentLoadedImplPrivate {
};


DocumentLoadedImpl::DocumentLoadedImpl(Document* document)
: AbstractDocumentImpl(document)
, d(new DocumentLoadedImplPrivate) {
}


DocumentLoadedImpl::~DocumentLoadedImpl() {
	delete d;
}


void DocumentLoadedImpl::init() {
}


bool DocumentLoadedImpl::isLoaded() const {
	return true;
}


bool DocumentLoadedImpl::saveInternal(QIODevice* device, const QByteArray& format) {
	bool ok = document()->image().save(device, format);
	if (ok) {
		setDocumentFormat(format);
	}
	return ok;
}


Document::SaveResult DocumentLoadedImpl::save(const KUrl& url, const QByteArray& format) {
	// FIXME: Handle remote urls
	Q_ASSERT(url.isLocalFile());
	KSaveFile file(url.path());

	if (!file.open()) {
		kWarning() << "Couldn't open" << url.pathOrUrl() << "for writing, probably read only";
		return Document::SR_ReadOnly;
	}

	if (saveInternal(&file, format)) {
		return Document::SR_OK;
	} else {
		kWarning() << "Saving" << url.pathOrUrl() << "failed";
		file.abort();
		return Document::SR_OtherError;
	}
}


void DocumentLoadedImpl::setImage(const QImage& image) {
	setDocumentImage(image);
	document()->setModified(true);
	imageRectUpdated(image.rect());
}


void DocumentLoadedImpl::applyTransformation(Orientation orientation) {
	QImage image = document()->image();
	QMatrix matrix = ImageUtils::transformMatrix(orientation);
	image = image.transformed(matrix);
	setDocumentImage(image);
	document()->setModified(true);
	imageRectUpdated(image.rect());
}

} // namespace
