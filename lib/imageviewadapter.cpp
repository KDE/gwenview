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
#include "imageviewadapter.moc"

// Qt

// KDE
#include <kurl.h>

// Local
#include <lib/imageview.h>
#include <lib/document/documentfactory.h>

namespace Gwenview {


struct ImageViewAdapterPrivate {
	ImageViewAdapter* that;
	ImageView* mView;
};


ImageViewAdapter::ImageViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new ImageViewAdapterPrivate) {
	d->mView = new ImageView(parent);
	setWidget(d->mView);

	connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)) );
}


ImageViewAdapter::~ImageViewAdapter() {
	delete d;
}


ImageView* ImageViewAdapter::imageView() const {
	return d->mView;
}


void ImageViewAdapter::openUrl(const KUrl& url) {
	// FIXME: Port GVPart::openUrl()
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	d->mView->setDocument(doc);

	connect(doc.data(), SIGNAL(downSampledImageReady()), SLOT(slotLoaded()) );
	connect(doc.data(), SIGNAL(loaded(const KUrl&)), SLOT(slotLoaded()) );
	connect(doc.data(), SIGNAL(loadingFailed(const KUrl&)), SLOT(slotLoadingFailed()) );
	if (doc->loadingState() == Document::Loaded) {
		slotLoaded();
	} else if (doc->loadingState() == Document::LoadingFailed) {
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


void ImageViewAdapter::slotLoaded() {
	emit completed();
	if (d->mView->zoomToFit()) {
		emit resizeRequested(d->mView->document()->size());
	}

	// We don't want to emit completed() again if we receive another
	// downSampledImageReady() or loaded() signal from the current document.
	disconnect(d->mView->document().data(), 0, this, SLOT(slotLoaded()) );
}


void ImageViewAdapter::slotLoadingFailed() {
	d->mView->setDocument(Document::Ptr());
	emit completed();
	// FIXME: Move this to DocumentPanel
	#if 0
	QString msg = i18n("Could not load <filename>%1</filename>.", url().fileName());
	mErrorLabel->setText(msg);
	mErrorWidget->adjustSize();
	mErrorWidget->show();

	if (mStatusBarWidgetContainer) {
		mStatusBarWidgetContainer->hide();
	}
	#endif
}


} // namespace
