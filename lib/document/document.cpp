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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "document.moc"

// Qt
#include <QApplication>
#include <QImage>

// KDE
#include <kdebug.h>
#include <kurl.h>

// Local
#include "emptydocumentimpl.h"
#include "loadingdocumentimpl.h"

namespace Gwenview {


struct DocumentPrivate {
	AbstractDocumentImpl* mImpl;
	KUrl mUrl;
	QSize mSize;
	QImage mImage;
	Exiv2::Image::AutoPtr mExiv2Image;
	QByteArray mFormat;
	bool mModified;
};


Document::Document() 
: QObject()
, d(new DocumentPrivate) {
	d->mImpl = new EmptyDocumentImpl(this);
	d->mModified = false;
}


Document::~Document() {
	delete d->mImpl;
	delete d;
}


void Document::load(const KUrl& url) {
	d->mUrl = url;
	switchToImpl(new LoadingDocumentImpl(this));
}


void Document::reload() {
	load(d->mUrl);
}


QImage& Document::image() {
	return d->mImage;
}


bool Document::isLoaded() const {
	return d->mImpl->isLoaded();
}


bool Document::isMetaDataLoaded() const {
	return d->mExiv2Image.get() && d->mSize.isValid() && !d->mFormat.isEmpty();
}


void Document::switchToImpl(AbstractDocumentImpl* impl) {
	// There should always be an implementation defined
	Q_ASSERT(d->mImpl);
	Q_ASSERT(impl);
	delete d->mImpl;
	d->mImpl=impl;

	connect(d->mImpl, SIGNAL(loaded()),
		this, SLOT(emitLoaded()) );
	connect(d->mImpl, SIGNAL(imageRectUpdated(const QRect&)),
		this, SIGNAL(imageRectUpdated(const QRect&)) );
	connect(d->mImpl, SIGNAL(metaDataLoaded()),
		this, SIGNAL(metaDataLoaded()) );
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
}


KUrl Document::url() const {
	return d->mUrl;
}


void Document::waitUntilLoaded() const {
	while (!isLoaded()) {
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}


Document::SaveResult Document::save(const KUrl& url, const QByteArray& format) {
	waitUntilLoaded();
	Document::SaveResult result = d->mImpl->save(url, format);
	if (result == SR_OK) {
		d->mModified = false;
		saved(url);
	}

	return result;
}

QByteArray Document::format() const {
	return d->mFormat;
}


void Document::setFormat(const QByteArray& format) {
	d->mFormat = format;
	emitMetaDataLoaded();
}


QSize Document::size() const {
	return d->mSize;
}


void Document::setSize(const QSize& size) {
	d->mSize = size;
	emitMetaDataLoaded();
}


bool Document::isModified() const {
	return d->mModified;
}

void Document::setModified(bool value) {
	d->mModified = value;
	if (d->mModified) {
		modified(d->mUrl);
	}
}


void Document::applyTransformation(Orientation orientation) {
	d->mImpl->applyTransformation(orientation);
}


const Exiv2::Image* Document::exiv2Image() const {
	return d->mExiv2Image.get();
}


void Document::setExiv2Image(Exiv2::Image::AutoPtr image) {
	d->mExiv2Image = image;
	emitMetaDataLoaded();
}


void Document::emitMetaDataLoaded() {
	if (isMetaDataLoaded()) {
		emit metaDataLoaded();
	}
}


void Document::emitLoaded() {
	emit loaded(d->mUrl);
}


} // namespace
