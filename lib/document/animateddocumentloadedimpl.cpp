// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "animateddocumentloadedimpl.moc"

// Qt
#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QMovie>

// KDE
#include <kdebug.h>

// Local

namespace Gwenview {


struct AnimatedDocumentLoadedImplPrivate {
	QByteArray mRawData;
	QBuffer mMovieBuffer;
	QMovie mMovie;
};


AnimatedDocumentLoadedImpl::AnimatedDocumentLoadedImpl(Document* document, const QByteArray& rawData)
: AbstractDocumentImpl(document)
, d(new AnimatedDocumentLoadedImplPrivate) {
	d->mRawData = rawData;

	connect(&d->mMovie, SIGNAL(frameChanged(int)), SLOT(slotFrameChanged(int)) );

	d->mMovieBuffer.setBuffer(&d->mRawData);
	d->mMovieBuffer.open(QIODevice::ReadOnly);
	d->mMovie.setDevice(&d->mMovieBuffer);
}


AnimatedDocumentLoadedImpl::~AnimatedDocumentLoadedImpl() {
	delete d;
}


void AnimatedDocumentLoadedImpl::init() {
	emit isAnimatedUpdated();
}


bool AnimatedDocumentLoadedImpl::isMetaInfoLoaded() const {
	return true;
}


Document::LoadingState AnimatedDocumentLoadedImpl::loadingState() const {
	return Document::Loaded;
}


void AnimatedDocumentLoadedImpl::setImage(const QImage&) {
	kWarning() << "This should not be called, we can't modify animated documents";
}


void AnimatedDocumentLoadedImpl::applyTransformation(Orientation) {
	kWarning() << "This should not be called, we can't modify animated documents";
}


QByteArray AnimatedDocumentLoadedImpl::rawData() const {
	return d->mRawData;
}


void AnimatedDocumentLoadedImpl::slotFrameChanged(int /*frameNumber*/) {
	QImage image = d->mMovie.currentImage();
	setDocumentImage(image);
	emit imageRectUpdated(image.rect());
}


bool AnimatedDocumentLoadedImpl::isAnimated() const {
	return true;
}


void AnimatedDocumentLoadedImpl::startAnimation() {
	d->mMovie.start();
	if (d->mMovie.state() == QMovie::NotRunning) {
		// This is true with qt-copy as of 2008.08.23
		kDebug() << "QMovie didn't start. This can happen in some cases when starting for the second time.";
		kDebug() << "Trying to start again, it usually fixes the bug.";
		d->mMovie.start();
	}
}


void AnimatedDocumentLoadedImpl::stopAnimation() {
	d->mMovie.stop();
}


} // namespace
