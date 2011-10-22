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
	QPixmap mCurrentBuffer;
	// The alternate buffer is useful when scrolling: existing content is copied
	// to mAlternateBuffer and buffers are swapped. This avoids allocating a new
	// QPixmap everytime the image is scrolled.
	QPixmap mAlternateBuffer;

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

	void updateBuffer(const QRegion& region = QRegion()) {
		mScaler->setZoom(q->zoom());
		if (region.isEmpty()) {
			setScalerRegionToVisibleRect();
		} else {
			mScaler->setDestinationRegion(region);
		}
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
	d->createBuffer();
	d->updateBuffer();
}

void RasterImageView::onImageOffsetChanged() {
	update();
}

void RasterImageView::onScrollPosChanged(const QPointF& oldPos) {
	d->updateBuffer();
	return;
#if 0
	int dx = oldPos.x() - imagePos().x();
	int dy = oldPos.y() - imagePos().y();

	// FIXME: QGV
	/*
	 *      if (d->mInsideSetZoom) {
	 *              // Do not scroll anything: since we are zooming the whole viewport will
	 *              // eventually be repainted
	 *              return;
	 }
	 */
	// Scroll existing
	{
		if (d->mAlternateBuffer.size() != d->mCurrentBuffer.size()) {
			d->mAlternateBuffer = QPixmap(d->mCurrentBuffer.size());
		}
		QPainter painter(&d->mAlternateBuffer);
		painter.drawPixmap(dx, dy, d->mCurrentBuffer);
	}
	qSwap(d->mCurrentBuffer, d->mAlternateBuffer);

	// Scale missing parts
	QRegion region;
	int posX = imagePos().x();
	int posY = imagePos().y();
	int width = size().width();
	int height = size().height();

	QRect rect;
	if (dx > 0) {
		rect = QRect(posX, posY, dx, height);
	} else {
		rect = QRect(posX + width + dx, posY, -dx, height);
	}
	region |= rect;

	if (dy > 0) {
		rect = QRect(posX, posY, width, dy);
	} else {
		rect = QRect(posX, posY + height + dy, width, -dy);
	}
	region |= rect;

	d->updateBuffer(region);
	update();
#endif
}

void RasterImageView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    painter->drawPixmap(imageOffset(), d->mCurrentBuffer);

	// FIXME: QGV
/*
	QSize bufferSize = d->mCurrentBuffer.size();

	QSizeF paintSize;
	if (d->mZoomToFit) {
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
*/

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
	Gwenview::AbstractImageView::resizeEvent(event);
	if (!zoomToFit()) {
		d->createBuffer();
		d->updateBuffer();
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
