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
#include <QScrollBar>

// KDE
#include <kdebug.h>

// Local
#include "../lib/imagescaler.h"


namespace Gwenview {


struct ImageViewPrivate {
	ImageView* mView;
	QWidget* mViewport;
	QImage mImage;
	qreal mZoom;
	bool mZoomToFit;
	QImage mBuffer;
	ImageScaler* mScaler;

	QPoint imageOffset() const {
		int left = qMax( (mViewport->width() - mBuffer.width()) / 2, 0);
		int top = qMax( (mViewport->height() - mBuffer.height()) / 2, 0);

		return QPoint(left, top);
	}

	qreal computeZoomToFit() const {
		int width = mViewport->width();
		int height = mViewport->height();
		qreal zoom = qreal(width) / mImage.width();
		if ( int(mImage.height() * zoom) > height) {
			zoom = qreal(height) / mImage.height();
		}
		return zoom;
	}

	QImage createBuffer() const {
		QSize visibleSize;
		qreal zoom;
		if (mZoomToFit) {
			zoom = computeZoomToFit();
		} else {
			zoom = mZoom;
		}

		visibleSize = mImage.size() * zoom;
		visibleSize = visibleSize.boundedTo(mViewport->size());

		return QImage(visibleSize, QImage::Format_ARGB32);
	}

	QRect mapViewportToZoomedImage(const QRect& viewportRect) {
		QRect rect = QRect(
			viewportRect.x() + mView->horizontalScrollBar()->value(),
			viewportRect.y() + mView->verticalScrollBar()->value(),
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
	d->mView = this;
	d->mZoom = 1.;
	d->mZoomToFit = true;
	setFrameShape(QFrame::NoFrame);
	d->mViewport = new QWidget();
	setViewport(d->mViewport);
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

void ImageView::setImage(const QImage& image) {
	d->mImage = image;
	d->mBuffer = d->createBuffer();
	d->mBuffer.fill(qRgb(0, 0, 0));
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	} else {
		updateScrollBars();
		startScaler();
	}
	d->mViewport->update();
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
	QPoint offset = d->imageOffset();

	// Erase pixels around the image
	QRect imageRect(offset, d->mBuffer.size());
	QRegion emptyRegion = QRegion(event->rect()) - QRegion(imageRect);
	Q_FOREACH(QRect rect, emptyRegion.rects()) {
		painter.fillRect(rect, Qt::black);
	}

	painter.drawImage(offset, d->mBuffer);
}

void ImageView::resizeEvent(QResizeEvent*) {
	QImage newBuffer = d->createBuffer();
	newBuffer.fill(qRgb(0, 0, 0));
	{
		QPainter painter(&newBuffer);
		painter.drawImage(0, 0, d->mBuffer);
	}
	d->mBuffer = newBuffer;
	if (d->mZoomToFit) {
		setZoom(d->computeZoomToFit());
	} else {
		updateScrollBars();
		startScaler();
	}
}

void ImageView::setZoom(qreal zoom) {
	d->mZoom = zoom;
	updateScrollBars();
	startScaler();
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
	} else {
		setZoom(1.);
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
	QImage newBuffer(d->mBuffer.size(), QImage::Format_ARGB32);
	newBuffer.fill(0);
	{
		QPainter painter(&newBuffer);
		painter.drawImage(dx, dy, d->mBuffer);
	}
	d->mBuffer = newBuffer;

	// Scale missing parts
	QRegion region;
	int posX = horizontalScrollBar()->value();
	int posY = verticalScrollBar()->value();
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
	left -= horizontalScrollBar()->value();
	top -= verticalScrollBar()->value();
	{
		QPainter painter(&d->mBuffer);
		painter.drawImage(left, top, image);
	}
	QPoint offset = d->imageOffset();
	d->mViewport->update(
		offset.x() + left,
		offset.y() + top,
		image.width(),
		image.height());
}


void ImageView::addTool(AbstractImageViewTool*) {
}


} // namespace
