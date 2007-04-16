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


ImageView::ImageView(QWidget* parent)
: QAbstractScrollArea(parent)
{
	mZoom = 1.;
	mZoomToFit = true;
	setFrameShape(QFrame::NoFrame);
	setViewport(new QWidget());
	viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
	horizontalScrollBar()->setSingleStep(16);
	verticalScrollBar()->setSingleStep(16);
	mScaler = new ImageScaler(this);
	mScaler->setTransformationMode(Qt::SmoothTransformation);
	connect(mScaler, SIGNAL(scaledRect(int, int, const QImage&)), 
		SLOT(updateFromScaler(int, int, const QImage&)) );
}

void ImageView::setImage(const QImage& image) {
	mImage = image;
	updateScrollBars();
	startScaler();
}

void ImageView::startScaler() {
	mScaler->setImage(mImage);
	mScaler->setZoom(mZoom);
	QRect rect(QPoint(0, 0), mImage.size() * mZoom);
	mScaler->setRegion(QRegion(rect));
}

void ImageView::paintEvent(QPaintEvent* event) {
	QPainter painter(viewport());
	painter.setClipRect(event->rect());
	painter.drawImage(0, 0, mBuffer);
}

void ImageView::resizeEvent(QResizeEvent*) {
	QImage tmp = mBuffer.copy(0, 0, viewport()->width(), viewport()->height());
	mBuffer = QImage(viewport()->size(), QImage::Format_ARGB32);
	{
		QPainter painter(&mBuffer);
		painter.drawImage(0, 0, tmp);
	}
	if (mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		updateScrollBars();
		startScaler();
	}
}

void ImageView::setZoom(qreal zoom) {
	mZoom = zoom;
	updateScrollBars();
	startScaler();
}

qreal ImageView::zoom() const {
	return mZoom;
}

bool ImageView::zoomToFit() const {
	return mZoomToFit;
}

void ImageView::setZoomToFit(bool on) {
	mZoomToFit = on;
	if (mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		setZoom(1.);
	}
}

void ImageView::updateScrollBars() {
	if (mZoomToFit) {
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		return;
	}
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	int max;
	int width = viewport()->width();
	int height = viewport()->height();

	max = qMax(0, int(mImage.width() * mZoom) - width);
	horizontalScrollBar()->setRange(0, max);
	horizontalScrollBar()->setPageStep(width);

	max = qMax(0, int(mImage.height() * mZoom) - height);
	verticalScrollBar()->setRange(0, max);
	verticalScrollBar()->setPageStep(height);
}

qreal ImageView::computeZoomToFit() const {
	int width = viewport()->width();
	int height = viewport()->height();
	qreal zoom = qreal(width) / mImage.width();
	if ( int(mImage.height() * zoom) > height) {
		zoom = qreal(height) / mImage.height();
	}
	return zoom;
}


void ImageView::scrollContentsBy(int dx, int dy) {
	// Scroll existing
	QImage newBuffer(mBuffer.size(), QImage::Format_ARGB32_Premultiplied);
	newBuffer.fill(0);
	{
		QPainter painter(&newBuffer);
		painter.drawImage(dx, dy, mBuffer);
	}
	mBuffer = newBuffer;

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

	mScaler->addRegion(region);
	viewport()->update();
}


void ImageView::updateFromScaler(int left, int top, const QImage& image) {
	left -= horizontalScrollBar()->value();
	top -= verticalScrollBar()->value();
	{
		QPainter painter(&mBuffer);
		painter.drawImage(left, top, image);
	}
	viewport()->update(left, top, image.width(), image.height());
}


} // namespace
