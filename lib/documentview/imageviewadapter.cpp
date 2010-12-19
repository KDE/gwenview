// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "imageviewadapter.moc"

// Qt

// KDE
#include <kurl.h>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/imageview.h>
#include <lib/scrolltool.h>
#include <lib/document/documentfactory.h>

namespace Gwenview {


struct ImageViewAdapterPrivate {
	ImageViewAdapter* that;
	ImageView* mView;
	ScrollTool* mScrollTool;

	void setupScrollTool() {
		mScrollTool = new ScrollTool(mView);
		mView->setDefaultTool(mScrollTool);
		QObject::connect(mScrollTool, SIGNAL(previousImageRequested()),
			that, SIGNAL(previousImageRequested()) );
		QObject::connect(mScrollTool, SIGNAL(nextImageRequested()),
			that, SIGNAL(nextImageRequested()) );
		QObject::connect(mScrollTool, SIGNAL(zoomInRequested(const QPoint&)),
			that, SIGNAL(zoomInRequested(const QPoint&)) );
		QObject::connect(mScrollTool, SIGNAL(zoomOutRequested(const QPoint&)),
			that, SIGNAL(zoomOutRequested(const QPoint&)) );
	}
};


ImageViewAdapter::ImageViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new ImageViewAdapterPrivate) {
	d->that = this;
	d->mView = new ImageView(parent);
	setWidget(d->mView);
	d->setupScrollTool();

	connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)) );
}


void ImageViewAdapter::installEventFilterOnViewWidgets(QObject* object) {
	d->mView->viewport()->installEventFilter(object);
}


ImageViewAdapter::~ImageViewAdapter() {
	delete d;
}


ImageView* ImageViewAdapter::imageView() const {
	return d->mView;
}


void ImageViewAdapter::setDocument(Document::Ptr doc) {
	d->mView->setDocument(doc);

	connect(doc.data(), SIGNAL(loadingFailed(const KUrl&)), SLOT(slotLoadingFailed()) );
	if (doc->loadingState() == Document::LoadingFailed) {
		slotLoadingFailed();
	}
}


qreal ImageViewAdapter::zoom() const {
	return d->mView->zoom();
}


void ImageViewAdapter::setZoomToFit(bool on) {
	d->mView->setZoomToFit(on);
}


bool ImageViewAdapter::zoomToFit() const {
	return d->mView->zoomToFit();
}


void ImageViewAdapter::setZoom(qreal zoom, const QPoint& center) {
	d->mView->setZoom(zoom, center);
}


qreal ImageViewAdapter::computeZoomToFit() const {
	return d->mView->computeZoomToFit();
}


qreal ImageViewAdapter::computeZoomToFitWidth() const {
	return d->mView->computeZoomToFitWidth();
}


qreal ImageViewAdapter::computeZoomToFitHeight() const {
	return d->mView->computeZoomToFitHeight();
}


Document::Ptr ImageViewAdapter::document() const {
	return d->mView->document();
}


void ImageViewAdapter::slotLoadingFailed() {
	d->mView->setDocument(Document::Ptr());
}


void ImageViewAdapter::loadConfig() {
	d->mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
	d->mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
	d->mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
}


} // namespace
