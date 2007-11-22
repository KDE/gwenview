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
#include "imageview.moc"

// Qt
#include <QPainter>
#include <QPaintEvent>
#include <QPointer>
#include <QScrollBar>

// KDE
#include <kdebug.h>

// Local
#include "abstractimageviewtool.h"
#include "imagescaler.h"


namespace Gwenview {


struct ImageViewPrivate {
	ImageView* mView;
	QPixmap mBackgroundTexture;
	QWidget* mViewport;
	ImageView::AlphaBackgroundMode mAlphaBackgroundMode;
	QColor mAlphaBackgroundColor;
	QImage mImage;
	qreal mZoom;
	bool mZoomToFit;
	QImage mBuffer;
	ImageScaler* mScaler;
	QPointer<AbstractImageViewTool> mTool;


	void createBackgroundTexture() {
		mBackgroundTexture = QPixmap(32, 32);
		QPainter painter(&mBackgroundTexture);
		painter.fillRect(mBackgroundTexture.rect(), QColor(128, 128, 128));
		QColor light = QColor(192, 192, 192);
		painter.fillRect(0, 0, 16, 16, light);
		painter.fillRect(16, 16, 16, 16, light);
	}


	qreal computeZoomToFit() const {
		int width = mViewport->width();
		int height = mViewport->height();
		qreal zoom = qreal(width) / mImage.width();
		if ( int(mImage.height() * zoom) > height) {
			zoom = qreal(height) / mImage.height();
		}
		return qMin(zoom, 1.0);
	}


	QSize requiredBufferSize() const {
		QSize size;
		qreal zoom;
		if (mZoomToFit) {
			zoom = computeZoomToFit();
		} else {
			zoom = mZoom;
		}

		size = mImage.size() * zoom;
		size = size.boundedTo(mViewport->size());

		return size;
	}


	QImage createBuffer() const {
		QSize size = requiredBufferSize();
		QImage buffer(size, QImage::Format_ARGB32);
		QColor bgColor = mView->palette().color(mView->backgroundRole());
		buffer.fill(bgColor.rgba());
		return buffer;
	}


	void resizeBuffer() {
		QSize size = requiredBufferSize();
		if (size == mBuffer.size()) {
			return;
		}
		QImage newBuffer = createBuffer();
		{
			QPainter painter(&newBuffer);
			painter.drawImage(0, 0, mBuffer);
		}
		mBuffer = newBuffer;
	}


	int hScroll() const {
		if (mZoomToFit) {
			return 0;
		} else {
			return mView->horizontalScrollBar()->value();
		}
	}

	int vScroll() const {
		if (mZoomToFit) {
			return 0;
		} else {
			return mView->verticalScrollBar()->value();
		}
	}

	QRect mapViewportToZoomedImage(const QRect& viewportRect) {
		QRect rect = QRect(
			viewportRect.x() + hScroll(),
			viewportRect.y() + vScroll(),
			viewportRect.width(),
			viewportRect.height()
		);

		return rect;
	}
};


ImageView::ImageView(QWidget* parent)
: QAbstractScrollArea(parent)
, d(new ImageViewPrivate)
{
	d->mAlphaBackgroundMode = AlphaBackgroundCheckBoard;
	d->mAlphaBackgroundColor = Qt::black;

	d->mView = this;
	d->mZoom = 1.;
	d->mZoomToFit = true;
	d->createBackgroundTexture();
	setFrameShape(QFrame::NoFrame);
	setBackgroundRole(QPalette::Base);
	d->mViewport = new QWidget();
	setViewport(d->mViewport);
	d->mViewport->setMouseTracking(true);
	d->mViewport->setAttribute(Qt::WA_OpaquePaintEvent, true);
	horizontalScrollBar()->setSingleStep(16);
	verticalScrollBar()->setSingleStep(16);
	d->mScaler = new ImageScaler(this);
	d->mScaler->setTransformationMode(Qt::SmoothTransformation);
	connect(d->mScaler, SIGNAL(scaledRect(int, int, const QImage&)), 
		SLOT(updateFromScaler(int, int, const QImage&)) );
}

ImageView::~ImageView() {
	delete d;
}


void ImageView::setAlphaBackgroundMode(AlphaBackgroundMode mode) {
	d->mAlphaBackgroundMode = mode;
	d->mViewport->update();
}


void ImageView::setAlphaBackgroundColor(const QColor& color) {
	d->mAlphaBackgroundColor = color;
	d->mViewport->update();
}


void ImageView::setImage(const QImage& image) {
	d->mImage = image;
	d->mBuffer = d->createBuffer();
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	} else {
		updateScrollBars();
		startScaler();
	}
	d->mViewport->update();
}


QImage ImageView::image() const {
	return d->mImage;
}


void ImageView::updateImageRect(const QRect& imageRect) {
	QRect viewportRect = mapToViewport(imageRect);
	d->mViewport->update(viewportRect);
}


void ImageView::startScaler() {
	d->mScaler->setImage(d->mImage);
	d->mScaler->setZoom(d->mZoom);

	QRect rect = d->mapViewportToZoomedImage(d->mViewport->rect());
	d->mScaler->setDestinationRegion(QRegion(rect));
}

void ImageView::paintEvent(QPaintEvent* event) {
	QPainter painter(d->mViewport);
	painter.setClipRect(event->rect());
	QPoint offset = imageOffset();

	// Erase pixels around the image
	QRect imageRect(offset, d->mBuffer.size());
	QRegion emptyRegion = QRegion(event->rect()) - QRegion(imageRect);
	QColor bgColor = palette().color(backgroundRole());
	Q_FOREACH(QRect rect, emptyRegion.rects()) {
		painter.fillRect(rect, bgColor);
	}

	if (d->mImage.hasAlphaChannel()) {
		if (d->mAlphaBackgroundMode == AlphaBackgroundCheckBoard) {
			painter.drawTiledPixmap(imageRect, d->mBackgroundTexture
				// This option makes the background scroll with the image, like GIMP
				// and others do. I think having a fixed background makes it easier to
				// distinguish transparent parts, so I comment it out for now.

				//, QPoint(d->hScroll() % d->mBackgroundTexture.width(), d->vScroll() % d->mBackgroundTexture.height())
				);
		} else {
			painter.fillRect(imageRect, d->mAlphaBackgroundColor);
		}
	}
	painter.drawImage(offset, d->mBuffer);

	if (d->mTool) {
		d->mTool->paint(&painter);
	}
}

void ImageView::resizeEvent(QResizeEvent*) {
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	} else {
		d->resizeBuffer();
		updateScrollBars();
		startScaler();
	}
}

QPoint ImageView::imageOffset() const {
	int left = qMax( (d->mViewport->width() - d->mBuffer.width()) / 2, 0);
	int top = qMax( (d->mViewport->height() - d->mBuffer.height()) / 2, 0);

	return QPoint(left, top);
}

void ImageView::setZoom(qreal zoom) {
	QPoint center = mapToImage(QPoint(d->mViewport->width() / 2, d->mViewport->height() / 2));
	setZoom(zoom, center);
}


void ImageView::setZoom(qreal zoom, const QPoint& center) {
	qreal oldZoom = d->mZoom;
	d->mZoom = zoom;
	d->resizeBuffer();
	if (d->mZoom < oldZoom && (d->mBuffer.width() < d->mViewport->width() || d->mBuffer.height() < d->mViewport->height())) {
		// Trigger an update to erase borders
		d->mViewport->update();
	}

	updateScrollBars();

	int hScrollValue = int(center.x() * d->mZoom) - d->mViewport->width() / 2;
	int vScrollValue = int(center.y() * d->mZoom) - d->mViewport->height() / 2;
	horizontalScrollBar()->setValue(hScrollValue);
	verticalScrollBar()->setValue(vScrollValue);

	startScaler();
	emit zoomChanged();
}

qreal ImageView::zoom() const {
	return d->mZoom;
}

bool ImageView::zoomToFit() const {
	return d->mZoomToFit;
}

void ImageView::setZoomToFit(bool on) {
	d->mZoomToFit = on;
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	}
}

void ImageView::updateScrollBars() {
	if (d->mZoomToFit) {
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		return;
	}
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	int max;
	int width = d->mViewport->width();
	int height = d->mViewport->height();

	max = qMax(0, int(d->mImage.width() * d->mZoom) - width);
	horizontalScrollBar()->setRange(0, max);
	horizontalScrollBar()->setPageStep(width);

	max = qMax(0, int(d->mImage.height() * d->mZoom) - height);
	verticalScrollBar()->setRange(0, max);
	verticalScrollBar()->setPageStep(height);
}


void ImageView::scrollContentsBy(int dx, int dy) {
	// Scroll existing
	// FIXME: Could be optimized a bit further by looping on the image rows one
	// time only.
	uchar* src;
	uchar* dst;
	int bpl = d->mBuffer.bytesPerLine();
	int delta;
	if (dy != 0) {
		if (dy > 0) {
			dst = d->mBuffer.bits() + (d->mBuffer.height() - 1) * bpl;
			src = dst - dy * bpl;
			delta = -bpl;
		} else {
			dst = d->mBuffer.bits();
			src = dst - dy * bpl;
			delta = bpl;
		}
		for (int loop=0; loop < d->mBuffer.height() - qAbs(dy); ++loop, src += delta, dst += delta) {
			memcpy(dst, src, bpl);
		}
	}

	if (dx != 0) {
		delta = bpl;
		if (dx > 0) {
			src = d->mBuffer.bits();
			dst = src + dx * 4;
		} else {
			dst = d->mBuffer.bits();
			src = dst - dx * 4;
		}
		int moveSize = (d->mBuffer.width() - qAbs(dx)) * 4;
		if (moveSize > 0) {
			for (int loop=0; loop < d->mBuffer.height(); ++loop, src += delta, dst += delta) {
				memmove(dst, src, moveSize);
			}
		}
	}

	// Scale missing parts
	QRegion region;
	int posX = d->hScroll();
	int posY = d->vScroll();
	int width = d->mViewport->width();
	int height = d->mViewport->height();

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

	d->mScaler->addDestinationRegion(region);
	d->mViewport->update();
}


void ImageView::updateFromScaler(int left, int top, const QImage& image) {
	left -= d->hScroll();
	top -= d->vScroll();

	{
		QPainter painter(&d->mBuffer);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(left, top, image);
	}
	QPoint offset = imageOffset();
	d->mViewport->update(
		offset.x() + left,
		offset.y() + top,
		image.width(),
		image.height());
}


void ImageView::setCurrentTool(AbstractImageViewTool* tool) {
	if (d->mTool) {
		d->mTool->toolDeactivated();
	}
	d->mTool = tool;
	if (d->mTool) {
		d->mTool->toolActivated();
	}
	d->mViewport->update();
}


AbstractImageViewTool* ImageView::currentTool() const {
	return d->mTool;
}


QPoint ImageView::mapToViewport(const QPoint& src) {
	QPoint dst(int(src.x() * d->mZoom), int(src.y() * d->mZoom));

	dst += imageOffset();

	dst.rx() -= d->hScroll();
	dst.ry() -= d->vScroll();

	return dst;
}


QRect ImageView::mapToViewport(const QRect& src) {
	QRect dst(
		mapToViewport(src.topLeft()),
		mapToViewport(src.bottomRight())
	);
	return dst;
}


QPoint ImageView::mapToImage(const QPoint& src) {
	QPoint dst = src;
	
	dst.rx() += d->hScroll();
	dst.ry() += d->vScroll();

	dst -= imageOffset();

	return QPoint(int(dst.x() / d->mZoom), int(dst.y() / d->mZoom));
}


QRect ImageView::mapToImage(const QRect& src) {
	QRect dst(
		mapToImage(src.topLeft()),
		mapToImage(src.bottomRight())
	);
	return dst;
}


void ImageView::mousePressEvent(QMouseEvent* event) {
	if (d->mTool) {
		d->mTool->mousePressEvent(event);
	}
}


void ImageView::mouseMoveEvent(QMouseEvent* event) {
	if (d->mTool) {
		d->mTool->mouseMoveEvent(event);
	}
}


void ImageView::mouseReleaseEvent(QMouseEvent* event) {
	if (d->mTool) {
		d->mTool->mouseReleaseEvent(event);
	}
}


} // namespace
