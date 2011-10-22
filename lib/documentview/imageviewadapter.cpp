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
#include <QTimer>

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
	bool mBufferIsEmpty;
	QPixmap mCurrentBuffer;
	// The alternate buffer is useful when scrolling: existing content is copied
	// to mAlternateBuffer and buffers are swapped. This avoids allocating a new
	// QPixmap everytime the image is scrolled.
	QPixmap mAlternateBuffer;

	QTimer* mUpdateTimer;

	void setupUpdateTimer() {
		mUpdateTimer = new QTimer(q);
		mUpdateTimer->setInterval(500);
		mUpdateTimer->setSingleShot(true);
		QObject::connect(mUpdateTimer, SIGNAL(timeout()),
			q, SLOT(updateBuffer()));
	}

	void startAnimationIfNecessary() {
	}

	QSizeF visibleImageSize() const {
		if (!q->document()) {
			return QSizeF();
		}
		qreal zoom;
		if (q->zoomToFit()) {
			zoom = q->computeZoomToFit();
		} else {
			zoom = q->zoom();
		}

		QSizeF size = q->documentSize() * zoom;
		return size.boundedTo(q->boundingRect().size());
	}

	QRectF mapViewportToZoomedImage(const QRectF& viewportRect) const {
		return QRectF(
			viewportRect.topLeft() - q->imageOffset() + q->scrollPos(),
			viewportRect.size()
			);
	}

	void setScalerRegionToVisibleRect() {
		QRectF rect = mapViewportToZoomedImage(q->boundingRect());
		mScaler->setDestinationRegion(QRegion(rect.toRect()));
	}

	void createBuffer() {
		QSize size = visibleImageSize().toSize();
		if (size == mCurrentBuffer.size()) {
			return;
		}
		if (!size.isValid()) {
			mAlternateBuffer = QPixmap();
			mCurrentBuffer = QPixmap();
			return;
		}

		mAlternateBuffer = QPixmap(size);
		mAlternateBuffer.fill(Qt::transparent);
		{
			QPainter painter(&mAlternateBuffer);
			painter.drawPixmap(0, 0, mCurrentBuffer);
		}
		qSwap(mAlternateBuffer, mCurrentBuffer);

		mAlternateBuffer = QPixmap();
	}
};

RasterImageView::RasterImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, d(new RasterImageViewPrivate) {
	d->q = this;
	d->mBufferIsEmpty = true;
	d->mScaler = new ImageScaler(this);
	connect(d->mScaler, SIGNAL(scaledRect(int,int,QImage)), 
		SLOT(updateFromScaler(int,int,QImage)) );

	d->setupUpdateTimer();
}

RasterImageView::~RasterImageView() {
	delete d;
}

void RasterImageView::loadFromDocument() {
	Document::Ptr doc = document();
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

	d->createBuffer();
	d->mScaler->setDocument(document());

	connect(document().data(), SIGNAL(imageRectUpdated(QRect)),
		SLOT(updateImageRect(QRect)) );

	if (zoomToFit()) {
		setZoom(computeZoomToFit());
	} else {
		QRect rect(QPoint(0, 0), document()->size());
		updateImageRect(rect);
	}

	d->startAnimationIfNecessary();
	update();
}

void RasterImageView::updateImageRect(const QRect& /*imageRect*/) {
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

void RasterImageView::updateFromScaler(int zoomedImageLeft, int zoomedImageTop, const QImage& image) {
	int viewportLeft = zoomedImageLeft - scrollPos().x();
	int viewportTop = zoomedImageTop - scrollPos().y();
	d->mBufferIsEmpty = false;
	{
		QPainter painter(&d->mCurrentBuffer);
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

void RasterImageView::onZoomChanged() {
	// If we zoom more than twice, then assume the user wants to see the real
	// pixels, for example to fine tune a crop operation
	if (zoom() < 2.) {
		d->mScaler->setTransformationMode(Qt::SmoothTransformation);
	} else {
		d->mScaler->setTransformationMode(Qt::FastTransformation);
	}
	if (!d->mUpdateTimer->isActive()) {
		updateBuffer();
	}
}

void RasterImageView::onImageOffsetChanged() {
	update();
}

void RasterImageView::onScrollPosChanged(const QPointF& oldPos) {
	QPointF delta = scrollPos() - oldPos;

	// Scroll existing
	{
		if (d->mAlternateBuffer.size() != d->mCurrentBuffer.size()) {
			d->mAlternateBuffer = QPixmap(d->mCurrentBuffer.size());
		}
		QPainter painter(&d->mAlternateBuffer);
		painter.drawPixmap(-delta, d->mCurrentBuffer);
	}
	qSwap(d->mCurrentBuffer, d->mAlternateBuffer);

	// Scale missing parts
	QRegion bufferRegion = QRegion(d->mCurrentBuffer.rect().translated(scrollPos().toPoint()));
	QRegion updateRegion = bufferRegion - bufferRegion.translated(-delta.toPoint());
	updateBuffer(updateRegion);
	update();
}

void RasterImageView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
	QSize bufferSize = d->mCurrentBuffer.size();

	QSizeF paintSize;
	if (zoomToFit()) {
		paintSize = documentSize() * computeZoomToFit();
	} else {
		paintSize = bufferSize;
	}
	painter->drawPixmap(
		(boundingRect().width() - paintSize.width()) / 2,
		(boundingRect().height() - paintSize.height()) / 2,
		paintSize.width(),
		paintSize.height(),
		d->mCurrentBuffer);

	// Debug
#if 0
	QPointF topLeft = imageOffset();
	QSizeF visibleSize = documentSize() * zoom();
	painter->setPen(Qt::red);
	painter->drawRect(topLeft.x(), topLeft.y(), visibleSize.width() - 1, visibleSize.height() - 1);

	painter->setPen(Qt::blue);
	painter->drawRect(topLeft.x(), topLeft.y(), d->mCurrentBuffer.width() - 1, d->mCurrentBuffer.height() - 1);
#endif
}

void RasterImageView::resizeEvent(QGraphicsSceneResizeEvent* event) {
	// If we are in zoomToFit mode and have something in our buffer, delay the
	// update: paint() will paint a scaled version of the buffer until resizing
	// is done. This is much faster than rescaling the whole image for each
	// resize event we receive.
	// mUpdateTimer must be start before calling AbstractImageView::resizeEvent()
	// because AbstractImageView::resizeEvent() will call onZoomChanged(), which
	// will trigger an immediate update unless the mUpdateTimer is active.
	if (zoomToFit() && !d->mBufferIsEmpty) {
		d->mUpdateTimer->start();
	}
	AbstractImageView::resizeEvent(event);
	if (!zoomToFit()) {
		// Only update buffer if we are not in zoomToFit mode: if we are
		// onZoomChanged() will have already updated the buffer.
		updateBuffer();
	}
}

void RasterImageView::updateBuffer(const QRegion& region) {
	d->mUpdateTimer->stop();
	d->createBuffer();
	d->mScaler->setZoom(zoom());
	if (region.isEmpty()) {
		d->setScalerRegionToVisibleRect();
	} else {
		d->mScaler->setDestinationRegion(region);
	}
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

	*/
	d->mView = new RasterImageView;
	connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)) );
	connect(d->mView, SIGNAL(zoomToFitChanged(bool)), SIGNAL(zoomToFitChanged(bool)) );
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
