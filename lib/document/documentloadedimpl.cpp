// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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

// STL
#include <memory>

// Qt
#include <QByteArray>
#include <QImage>
#include <QImageWriter>
#include <QMatrix>

// KDE
#include <kdebug.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <ksavefile.h>
#include <ktemporaryfile.h>
#include <kurl.h>

// Local
#include "imageutils.h"

namespace Gwenview {


struct DocumentLoadedImplPrivate {
	QByteArray mRawData;
};


DocumentLoadedImpl::DocumentLoadedImpl(Document* document, const QByteArray& rawData)
: AbstractDocumentImpl(document)
, d(new DocumentLoadedImplPrivate) {
	if (document->keepRawData()) {
		d->mRawData = rawData;
	}
}


DocumentLoadedImpl::~DocumentLoadedImpl() {
	delete d;
}


void DocumentLoadedImpl::init() {
}


bool DocumentLoadedImpl::isEditable() const {
	return true;
}


bool DocumentLoadedImpl::isMetaInfoLoaded() const {
	return true;
}


Document::LoadingState DocumentLoadedImpl::loadingState() const {
	return Document::Loaded;
}


bool DocumentLoadedImpl::saveInternal(QIODevice* device, const QByteArray& format) {
	QImageWriter writer(device, format);
	bool ok = writer.write(document()->image());
	if (ok) {
		setDocumentFormat(format);
	} else {
		setDocumentErrorString(writer.errorString());
	}
	return ok;
}


bool DocumentLoadedImpl::save(const KUrl& url, const QByteArray& format) {
	QString fileName;

	// This tmp is used to save to remote urls.
	// It's an auto_ptr, this way it's not instantiated for local urls, but
	// for remote urls we are sure it will remove its file when we leave the
	// function.
	std::auto_ptr<KTemporaryFile> tmp;

	if (url.isLocalFile()) {
		fileName = url.toLocalFile();
	} else {
		tmp.reset(new KTemporaryFile);
		tmp->setAutoRemove(true);
		tmp->open();
		fileName = tmp->fileName();
	}

	KSaveFile file(fileName);

	if (!file.open()) {
		KUrl dirUrl = url;
		dirUrl.setFileName(QString());
		setDocumentErrorString(i18nc("@info", "Could not open file for writing, check that you have the necessary rights in <filename>%1</filename>.", dirUrl.pathOrUrl()));
		return false;
	}

	if (!saveInternal(&file, format)) {
		file.abort();
		return false;
	}

	if (!file.finalize()) {
		setDocumentErrorString(i18nc("@info", "Could not overwrite file, check that you have the necessary rights to write in <filename>%1</filename>.", url.pathOrUrl()));
		return false;
	}

	if (!url.isLocalFile()) {
		if (!KIO::NetAccess::upload(fileName, url, 0)) {
			setDocumentErrorString(i18nc("@info", "Could not upload file."));
			return false;
		}
	}

	return true;
}


AbstractDocumentEditor* DocumentLoadedImpl::editor() {
	return this;
}


void DocumentLoadedImpl::setImage(const QImage& image) {
	setDocumentImage(image);
	imageRectUpdated(image.rect());
}


void DocumentLoadedImpl::applyTransformation(Orientation orientation) {
	QImage image = document()->image();
	QMatrix matrix = ImageUtils::transformMatrix(orientation);
	image = image.transformed(matrix);
	setDocumentImage(image);
	imageRectUpdated(image.rect());
}


QByteArray DocumentLoadedImpl::rawData() const {
	return d->mRawData;
}


} // namespace
