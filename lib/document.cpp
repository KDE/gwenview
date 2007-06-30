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
#include <KUrl>

// Local
#include "emptydocumentimpl.h"
#include "loadingdocumentimpl.h"

namespace Gwenview {


struct DocumentPrivate {
	AbstractDocumentImpl* mImpl;
	KUrl mUrl;
	QImage mImage;
	QByteArray mFormat;
};


Document::Document() 
: QObject()
, d(new DocumentPrivate) {
    d->mImpl = new EmptyDocumentImpl(this);
}


Document::~Document() {
	delete d->mImpl;
	delete d;
}


void Document::load(const KUrl& url) {
	d->mUrl = url;
    switchToImpl(new LoadingDocumentImpl(this));
}


QImage& Document::image() {
	return d->mImage;
}

bool Document::isLoaded() const {
	return d->mImpl->isLoaded();
}


void Document::switchToImpl(AbstractDocumentImpl* impl) {
	// There should always be an implementation defined
	Q_ASSERT(d->mImpl);
	Q_ASSERT(impl);
	delete d->mImpl;
	d->mImpl=impl;

	connect(d->mImpl, SIGNAL(loaded()),
		this, SIGNAL(loaded()) );
	d->mImpl->init();
}


void Document::setImage(const QImage& image) {
	d->mImage = image;
}


KUrl Document::url() const {
	return d->mUrl;
}

Document::SaveResult Document::save(const KUrl& url, const QString& format) {
	while (!isLoaded()) {
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	return d->mImpl->save(url, format);
}

QByteArray Document::format() const {
	return d->mFormat;
}

void Document::setFormat(const QByteArray& format) {
	d->mFormat = format;
}

bool Document::isModified() const {
	return false;
}

} // namespace
