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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "document.moc"

// Qt
#include <QApplication>
#include <QImage>
#include <QUndoStack>

// KDE
#include <kdebug.h>
#include <kfileitem.h>
#include <kurl.h>

// Local
#include "emptydocumentimpl.h"
#include "imagemetainfomodel.h"
#include "loadingdocumentimpl.h"

namespace Gwenview {


struct DocumentPrivate {
	AbstractDocumentImpl* mImpl;
	KUrl mUrl;
	QSize mSize;
	QImage mImage;
	QMap<int, QImage> mDownSampledImageMap;
	Exiv2::Image::AutoPtr mExiv2Image;
	QByteArray mFormat;
	ImageMetaInfoModel mImageMetaInfoModel;
	QUndoStack mUndoStack;
};


Document::Document(const KUrl& url, Document::LoadType loadType)
: QObject()
, d(new DocumentPrivate) {
	d->mImpl = 0;
	d->mUrl = url;

	connect(&d->mUndoStack, SIGNAL(indexChanged(int)), SLOT(slotUndoIndexChanged()) );
	KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, url);
	d->mImageMetaInfoModel.setFileItem(fileItem);
	switchToImpl(new LoadingDocumentImpl(this, loadType));
}


Document::~Document() {
	delete d->mImpl;
	delete d;
}


void Document::finishLoading() {
	if (loadingState() == Loading) {
		LoadingDocumentImpl* impl = qobject_cast<LoadingDocumentImpl*>(d->mImpl);
		Q_ASSERT(impl);
		impl->finishLoading();
	}
}


void Document::reload() {
	d->mUndoStack.clear();
	KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, d->mUrl);
	d->mImageMetaInfoModel.setFileItem(fileItem);
	switchToImpl(new LoadingDocumentImpl(this, Document::LoadAll));
}


QImage& Document::image() {
	return d->mImage;
}


const QImage& Document::downSampledImage(qreal zoom) const {
	if (d->mImage.isNull()) {
		return d->mImage;
	}

	/*
	 * invertedZoom is the biggest power of 2 for which zoom < 1/invertedZoom.
	 * Example:
	 * zoom = 0.4 == 1/2.5 => invertedZoom = 2 (1/2.5 < 1/2)
	 * zoom = 0.2 == 1/5   => invertedZoom = 4 (1/5   < 1/4)
	*/
	int invertedZoom;
	for (invertedZoom = 1; zoom < 1./(invertedZoom*2); invertedZoom*=2);
	if (invertedZoom == 1) {
		return d->mImage;
	}

	if (!d->mDownSampledImageMap.contains(invertedZoom)) {
		d->mDownSampledImageMap[invertedZoom] = d->mImage.scaled(d->mImage.size() / invertedZoom, Qt::KeepAspectRatio, Qt::FastTransformation);
	}

	return d->mDownSampledImageMap[invertedZoom];
}


bool Document::isMetaDataLoaded() const {
	return d->mImpl->isMetaDataLoaded();
}


Document::LoadingState Document::loadingState() const {
	return d->mImpl->loadingState();
}


void Document::switchToImpl(AbstractDocumentImpl* impl) {
	Q_ASSERT(impl);
	if (d->mImpl) {
		d->mImpl->deleteLater();
	}
	d->mImpl=impl;

	connect(d->mImpl, SIGNAL(loaded()),
		this, SLOT(emitLoaded()) );
	connect(d->mImpl, SIGNAL(loadingFailed()),
		this, SLOT(emitLoadingFailed()) );
	connect(d->mImpl, SIGNAL(imageRectUpdated(const QRect&)),
		this, SIGNAL(imageRectUpdated(const QRect&)) );
	d->mImpl->init();
}


void Document::setImage(const QImage& image) {
	// Don't init mImage directly, because:
	// - This should not be called until document has finished loading.
	// - Some impl will want to do special stuff (ex: jpegloaded implementation will
	// switch to loaded implementation since it won't hold valid raw data
	// anymore)
	d->mImpl->setImage(image);
}


void Document::setImageInternal(const QImage& image) {
	d->mImage = image;
	d->mDownSampledImageMap.clear();

	// If we didn't get the image size before decoding the full image, set it
	// now
	setSize(d->mImage.size());
}


KUrl Document::url() const {
	return d->mUrl;
}


void Document::waitUntilMetaDataLoaded() const {
	while (!isMetaDataLoaded()) {
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}


void Document::waitUntilLoaded() const {
	while (loadingState() != Loaded) {
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}


Document::SaveResult Document::save(const KUrl& url, const QByteArray& format) {
	waitUntilLoaded();
	Document::SaveResult result = d->mImpl->save(url, format);
	if (result == SR_OK) {
		d->mUndoStack.setClean();
		saved(url);
	}

	return result;
}

QByteArray Document::format() const {
	return d->mFormat;
}


void Document::setFormat(const QByteArray& format) {
	d->mFormat = format;
	emit metaDataUpdated();
}


QSize Document::size() const {
	return d->mSize;
}


bool Document::hasAlphaChannel() const {
	if (d->mImage.isNull()) {
		return false;
	} else {
		return d->mImage.hasAlphaChannel();
	}
}


void Document::setSize(const QSize& size) {
	if (size == d->mSize) {
		return;
	}
	d->mSize = size;
	d->mImageMetaInfoModel.setImageSize(size);
	emit metaDataUpdated();
}


bool Document::isModified() const {
	return !d->mUndoStack.isClean();
}


void Document::applyTransformation(Orientation orientation) {
	d->mImpl->applyTransformation(orientation);
}


const Exiv2::Image* Document::exiv2Image() const {
	return d->mExiv2Image.get();
}


void Document::setExiv2Image(Exiv2::Image::AutoPtr image) {
	d->mExiv2Image = image;
	d->mImageMetaInfoModel.setExiv2Image(d->mExiv2Image.get());
	emit metaDataUpdated();
}


ImageMetaInfoModel* Document::metaInfo() const {
	return &d->mImageMetaInfoModel;
}


void Document::emitLoaded() {
	emit loaded(d->mUrl);
}


void Document::emitLoadingFailed() {
	emit loadingFailed(d->mUrl);
}


QUndoStack* Document::undoStack() const {
	return &d->mUndoStack;
}


void Document::slotUndoIndexChanged() {
	if (d->mUndoStack.isClean()) {
		// This does not really correspond to a save
		saved(d->mUrl);
	} else {
		modified(d->mUrl);
	}
}


} // namespace
