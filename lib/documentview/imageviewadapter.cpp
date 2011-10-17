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
#include <QGraphicsProxyWidget>
#include <QPainter>

// KDE
#include <kurl.h>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/imagescaler.h>
#include <lib/imageview.h>
#include <lib/scrolltool.h>
#include <lib/document/documentfactory.h>

namespace Gwenview {

struct RasterImageViewPrivate {
	RasterImageView* q;
	ImageScaler* mScaler;

	void startAnimationIfNecessary() {
	}

	void setScalerRegionToVisibleRect() {
		QRectF rect = q->mapViewportToZoomedImage(q->boundingRect());
		mScaler->setDestinationRegion(QRegion(rect.toRect()));
	}
};

RasterImageView::RasterImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, d(new RasterImageViewPrivate) {
	d->q = this;
	d->mScaler = new ImageScaler(this);
	connect(d->mScaler, SIGNAL(scaledRect(int,int,QImage)), 
		SLOT(updateFromScaler(int,int,QImage)) );
}

RasterImageView::~RasterImageView() {
	delete d;
}

void RasterImageView::setDocument(Document::Ptr doc) {
	AbstractImageView::setDocument(doc);
	connect(doc.data(), SIGNAL(metaInfoLoaded(KUrl)),
		SLOT(slotDocumentMetaInfoLoaded()) );
	connect(doc.data(), SIGNAL(isAnimatedUpdated()),
		SLOT(slotDocumentIsAnimatedUpdated()) );

	const Document::LoadingState state = doc->loadingState();
	if (state == Document::MetaInfoLoaded || state == Document::Loaded) {
		slotDocumentMetaInfoLoaded();
	}
}


void RasterImageView::slotDocumentMetaInfoLoaded() {
	if (document()->size().isValid()) {
		finishSetDocument();
	} else {
		// Could not retrieve image size from meta info, we need to load the
		// full image now.
		connect(document().data(), SIGNAL(loaded(KUrl)),
			SLOT(finishSetDocument()) );
		document()->startLoadingFullImage();
	}
}


void RasterImageView::finishSetDocument() {
	if (!document()->size().isValid()) {
		kError() << "No valid image size available, this should not happen!";
		return;
	}

	createBuffer();
	d->mScaler->setDocument(document());

	connect(document().data(), SIGNAL(imageRectUpdated(QRect)),
		SLOT(updateImageRect(QRect)) );

	if (zoomToFit()) {
		// Set the zoom to an invalid value to make sure setZoom() does not
		// return early because the new zoom is the same as the old zoom.
		// FIXME: QGV
		//d->mZoom = -1;
		setZoom(computeZoomToFit());
	} else {
		QRect rect(QPoint(0, 0), document()->size());
		updateImageRect(rect);
	}

	d->startAnimationIfNecessary();
	update();
}

void RasterImageView::updateImageRect(const QRect& imageRect) {
	// FIXME: QGV
	/*
	QRect viewportRect = mapToViewport(imageRect);
	viewportRect = viewportRect.intersected(d->mViewport->rect());
	if (viewportRect.isEmpty()) {
		return;
	}
	*/

	if (zoomToFit()) {
		setZoom(computeZoomToFit());
	}
	d->setScalerRegionToVisibleRect();
	update();
}

void RasterImageView::slotDocumentIsAnimatedUpdated() {
	d->startAnimationIfNecessary();
}


void RasterImageView::updateBuffer() {
	d->mScaler->setZoom(zoom());
	d->setScalerRegionToVisibleRect();
}

void RasterImageView::updateFromScaler(int zoomedImageLeft, int zoomedImageTop, const QImage& image) {
	// FIXME: QGV
	int viewportLeft = zoomedImageLeft; // - d->hScroll();
	int viewportTop = zoomedImageTop; // - d->vScroll();
	{
		QPainter painter(&buffer());
		/*
		if (d->mDocument->hasAlphaChannel()) {
			d->drawAlphaBackground(
				&painter, QRect(viewportLeft, viewportTop, image.width(), image.height()),
				QPoint(zoomedImageLeft, zoomedImageTop)
				);
		} else {
			painter.setCompositionMode(QPainter::CompositionMode_Source);
		}
		painter.drawImage(viewportLeft, viewportTop, image);
		*/
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(viewportLeft, viewportTop, image);
	}
	update();
}

void RasterImageView::setZoom(qreal zoom, const QPointF& _center) {
	// FIXME: QGV
#if 0
	if (!d->mDocument) {
		return;
	}

	qreal oldZoom = d->mZoom;
	if (qAbs(zoom - oldZoom) < 0.001) {
		return;
	}
	// Get offset *before* setting mZoom, otherwise we get the new offset
	QPoint oldOffset = imageOffset();
	d->mZoom = zoom;

	QPoint center;
	if (_center == QPoint(-1, -1)) {
		center = QPoint(d->mViewport->width() / 2, d->mViewport->height() / 2);
	} else {
		center = _center;
	}

	// If we zoom more than twice, then assume the user wants to see the real
	// pixels, for example to fine tune a crop operation
	if (d->mZoom < 2.) {
		d->mScaler->setTransformationMode(Qt::SmoothTransformation);
	} else {
		d->mScaler->setTransformationMode(Qt::FastTransformation);
	}

	d->createBuffer();
	d->mInsideSetZoom = true;

	/*
	We want to keep the point at viewport coordinates "center" at the same
	position after zooming. The coordinates of this point in image coordinates
	can be expressed like this:

	                      oldScroll + center
	imagePointAtOldZoom = ------------------
	                           oldZoom

	                   scroll + center
	imagePointAtZoom = ---------------
	                        zoom

	So we want:

	    imagePointAtOldZoom = imagePointAtZoom

	    oldScroll + center   scroll + center
	<=> ------------------ = ---------------
	          oldZoom             zoom

	              zoom
	<=> scroll = ------- (oldScroll + center) - center
	             oldZoom
	*/

	/*
	Compute oldScroll
	It's useless to take the new offset in consideration because if a direction
	of the new offset is not 0, we won't be able to center on a specific point
	in that direction.
	*/
	QPointF oldScroll = QPointF(d->hScroll(), d->vScroll()) - oldOffset;

	QPointF scroll = (zoom / oldZoom) * (oldScroll + center) - center;

	updateScrollBars();
	horizontalScrollBar()->setValue(int(scroll.x()));
	verticalScrollBar()->setValue(int(scroll.y()));
	d->mInsideSetZoom = false;

	d->mScaler->setZoom(d->mZoom);
	d->setScalerRegionToVisibleRect();
	emit zoomChanged(d->mZoom);
#endif
	AbstractImageView::setZoom(zoom, _center);
	updateBuffer();
}

//// ImageViewAdapter ////
struct ImageViewAdapterPrivate {
	ImageViewAdapter* that;
	RasterImageView* mView;
	/*
	ScrollTool* mScrollTool;

	void setupScrollTool() {
		mScrollTool = new ScrollTool(mView);
		mView->setDefaultTool(mScrollTool);
	}
	*/
};


ImageViewAdapter::ImageViewAdapter()
: d(new ImageViewAdapterPrivate) {
	d->that = this;
	/*
	d->mView = new ImageView;
	QGraphicsProxyWidget* proxyWidget = new QGraphicsProxyWidget;
	proxyWidget->setWidget(d->mView);
	setWidget(proxyWidget);
	d->setupScrollTool();

	connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)) );
	*/
	d->mView = new RasterImageView;
	setWidget(d->mView);
}


ImageViewAdapter::~ImageViewAdapter() {
	delete d;
}


ImageView* ImageViewAdapter::imageView() const {
	// FIXME: QGV
	//return d->mView;
	return 0;
}


QCursor ImageViewAdapter::cursor() const {
	return d->mView->cursor();
}


void ImageViewAdapter::setCursor(const QCursor& cursor) {
	d->mView->setCursor(cursor);
}


void ImageViewAdapter::setDocument(Document::Ptr doc) {
	d->mView->setDocument(doc);

	connect(doc.data(), SIGNAL(loadingFailed(KUrl)), SLOT(slotLoadingFailed()) );
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


void ImageViewAdapter::setZoom(qreal zoom, const QPointF& center) {
	d->mView->setZoom(zoom, center);
}


qreal ImageViewAdapter::computeZoomToFit() const {
	return d->mView->computeZoomToFit();
}


Document::Ptr ImageViewAdapter::document() const {
	return d->mView->document();
}


void ImageViewAdapter::slotLoadingFailed() {
	d->mView->setDocument(Document::Ptr());
}


void ImageViewAdapter::loadConfig() {
	// FIXME: QGV
	/*
	d->mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
	d->mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
	d->mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
	*/
}

} // namespace
