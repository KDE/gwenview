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

// Local
#include "../lib/imagescaler.h"


namespace Gwenview {


struct ImageViewPrivate {
	QImage mImage;
	qreal mZoom;
	bool mZoomToFit;
	QImage mBuffer;
	ImageScaler* mScaler;
};


ImageView::ImageView(QWidget* parent)
: QAbstractScrollArea(parent)
, d(new ImageViewPrivate)
{
	d->mZoom = 1.;
	d->mZoomToFit = true;
	setFrameShape(QFrame::NoFrame);
	setViewport(new QWidget());
	viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
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
	updateScrollBars();
	startScaler();
}

void ImageView::startScaler() {
	d->mScaler->setImage(d->mImage);
	d->mScaler->setZoom(d->mZoom);
	QRect rect(QPoint(0, 0), d->mImage.size() * d->mZoom);
	d->mScaler->setRegion(QRegion(rect));
}

void ImageView::paintEvent(QPaintEvent* event) {
	QPainter painter(viewport());
	painter.setClipRect(event->rect());
	painter.drawImage(0, 0, d->mBuffer);
}

void ImageView::resizeEvent(QResizeEvent*) {
	QImage tmp = d->mBuffer.copy(0, 0, viewport()->width(), viewport()->height());
	d->mBuffer = QImage(viewport()->size(), QImage::Format_ARGB32);
	{
		QPainter painter(&d->mBuffer);
		painter.drawImage(0, 0, tmp);
	}
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
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
		setZoom(computeZoomToFit());
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
	int width = viewport()->width();
	int height = viewport()->height();

	max = qMax(0, int(d->mImage.width() * d->mZoom) - width);
	horizontalScrollBar()->setRange(0, max);
	horizontalScrollBar()->setPageStep(width);

	max = qMax(0, int(d->mImage.height() * d->mZoom) - height);
	verticalScrollBar()->setRange(0, max);
	verticalScrollBar()->setPageStep(height);
}

qreal ImageView::computeZoomToFit() const {
	int width = viewport()->width();
	int height = viewport()->height();
	qreal zoom = qreal(width) / d->mImage.width();
	if ( int(d->mImage.height() * zoom) > height) {
		zoom = qreal(height) / d->mImage.height();
	}
	return zoom;
}


void ImageView::scrollContentsBy(int dx, int dy) {
	// Scroll existing
	QImage newBuffer(d->mBuffer.size(), QImage::Format_ARGB32_Premultiplied);
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
	int width = viewport()->width();
	int height = viewport()->height();

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

	d->mScaler->addRegion(region);
	viewport()->update();
}


void ImageView::updateFromScaler(int left, int top, const QImage& image) {
	left -= horizontalScrollBar()->value();
	top -= verticalScrollBar()->value();
	{
		QPainter painter(&d->mBuffer);
		painter.drawImage(left, top, image);
	}
	viewport()->update(left, top, image.width(), image.height());
}


} // namespace
