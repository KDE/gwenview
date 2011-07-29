// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "documentinfoprovider.moc"

// Qt
#include <QPixmap>

// KDE

// Local
#include <lib/document/documentfactory.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview {


struct DocumentInfoProviderPrivate {
	SortedDirModel* mDirModel;
};


DocumentInfoProvider::DocumentInfoProvider(SortedDirModel* model)
: AbstractDocumentInfoProvider(model)
, d(new DocumentInfoProviderPrivate) {
	d->mDirModel = model;
	connect(DocumentFactory::instance(), SIGNAL(documentBusyStateChanged(KUrl,bool)),
		SLOT(emitBusyStateChanged(KUrl,bool)) );

	connect(DocumentFactory::instance(), SIGNAL(documentChanged(KUrl)),
		SLOT(emitDocumentChanged(KUrl)) );
}


DocumentInfoProvider::~DocumentInfoProvider() {
	delete d;
}


void DocumentInfoProvider::thumbnailForDocument(const KUrl& url, ThumbnailGroup::Enum group, QPixmap* outPix, QSize* outFullSize) const {
	Q_ASSERT(outPix);
	Q_ASSERT(outFullSize);
	*outPix = QPixmap();
	*outFullSize = QSize();
	DocumentFactory* factory = DocumentFactory::instance();
	const int pixelSize = ThumbnailGroup::pixelSize(group);

	if (!factory->hasUrl(url)) {
		return;
	}

	Document::Ptr doc = factory->load(url);
	if (doc->loadingState() != Document::Loaded) {
		return;
	}

	QImage image = doc->image();
	if (image.width() > pixelSize || image.height() > pixelSize) {
		image = image.scaled(pixelSize, pixelSize, Qt::KeepAspectRatio);
	}
	*outPix = QPixmap::fromImage(image);
	*outFullSize = doc->size();
}


bool DocumentInfoProvider::isModified(const KUrl& url) {
	DocumentFactory* factory = DocumentFactory::instance();

	if (factory->hasUrl(url)) {
		Document::Ptr doc = factory->load(url);
		return doc->loadingState() == Document::Loaded && doc->isModified();
	} else {
		return false;
	}
}


bool DocumentInfoProvider::isBusy(const KUrl& url) {
	DocumentFactory* factory = DocumentFactory::instance();

	if (factory->hasUrl(url)) {
		Document::Ptr doc = factory->load(url);
		return doc->isBusy();
	} else {
		return false;
	}
}


void DocumentInfoProvider::emitBusyStateChanged(const KUrl& url, bool busy) {
	QModelIndex index = d->mDirModel->indexForUrl(url);
	if (!index.isValid()) {
		return;
	}
	busyStateChanged(index, busy);
}


void DocumentInfoProvider::emitDocumentChanged(const KUrl& url) {
	QModelIndex index = d->mDirModel->indexForUrl(url);
	if (!index.isValid()) {
		return;
	}
	documentChanged(index);
}


} // namespace
