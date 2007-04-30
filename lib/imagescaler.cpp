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
#include "imagescaler.moc"

#include <math.h>

// Qt
#include <QImage>
#include <QRegion>
#include <QTime>
#include <QTimer>

// KDE
#include <kdebug.h>

namespace Gwenview {

// Amount of pixels to keep so that smooth scale is correct
static const int SMOOTH_MARGIN = 8;

static const int MAX_CHUNK_AREA = 100 * 100;

static const int MAX_SCALE_TIME = 2000;

struct ImageScalerPrivate {
	Qt::TransformationMode mTransformationMode;
	QImage mImage;
	qreal mZoom;
	QRegion mRegion;
	QTimer* mTimer;
};

ImageScaler::ImageScaler(QObject* parent)
: QObject(parent)
, d(new ImageScalerPrivate) {
	d->mTransformationMode = Qt::FastTransformation;
	d->mTimer = new QTimer(this);
	connect(d->mTimer, SIGNAL(timeout()),
		SLOT(doScale()) );
}

ImageScaler::~ImageScaler() {
	delete d;
}

void ImageScaler::setImage(const QImage& image) {
	if (d->mTimer->isActive()) {
		d->mTimer->stop();
	}
	d->mImage = image;
}

void ImageScaler::setZoom(qreal zoom) {
	d->mZoom = zoom;
}

void ImageScaler::setTransformationMode(Qt::TransformationMode mode) {
	d->mTransformationMode = mode;
}

void ImageScaler::setRegion(const QRegion& region) {
	d->mRegion = region;
	if(!d->mRegion.isEmpty()) {
		d->mTimer->start();
	} else {
		d->mTimer->stop();
	}
}

void ImageScaler::addRegion(const QRegion& region) {
	d->mRegion |= region;
	if(!d->mRegion.isEmpty()) {
		d->mTimer->start();
	}
}

bool ImageScaler::isRunning() {
	return d->mTimer->isActive();
}

QRect ImageScaler::containingRect(const QRectF& rectF) {
	return QRect(
		QPoint(
			int(floor(rectF.left())),
			int(floor(rectF.top()))
			),
		QPoint(
			int(ceil(rectF.right() - 1.)),
			int(ceil(rectF.bottom() - 1.))
			)
		);
	// Note: QRect::right = left + width - 1, while QRectF::right = left + width
}

void ImageScaler::doScale() {
	Q_ASSERT(!d->mRegion.isEmpty());
	QTime chrono;
	chrono.start();
	while (chrono.elapsed() < MAX_SCALE_TIME && !d->mRegion.isEmpty()) {
		QRect rect = d->mRegion.rects()[0];
		if (rect.width() * rect.height() > MAX_CHUNK_AREA) {
			int height = qMax(1, MAX_CHUNK_AREA / rect.width());
			rect.setHeight(height);
		}
		d->mRegion -= rect;
		if (d->mRegion.isEmpty()) {
			d->mTimer->stop();
		}
		processChunk(rect);
	}
}

void ImageScaler::processChunk(const QRect& rect) {
	// If rect contains "half" pixels, make sure sourceRect includes them
	QRectF sourceRectF(
		rect.left() / d->mZoom,
		rect.top() / d->mZoom,
		rect.width() / d->mZoom,
		rect.height() / d->mZoom);

	QRect sourceRect = containingRect(sourceRectF);
	sourceRect = sourceRect.intersected(d->mImage.rect());
	Q_ASSERT(!sourceRect.isEmpty());

	// destRect is almost like rect, but it contains only "full" pixels
	QRect destRect = QRect(
		int(sourceRect.left() * d->mZoom),
		int(sourceRect.top() * d->mZoom),
		qMax(int(sourceRect.width() * d->mZoom), 1),
		qMax(int(sourceRect.height() * d->mZoom), 1)
		);

	QImage tmp;
	if (d->mTransformationMode == Qt::FastTransformation) {
		tmp = d->mImage.copy(sourceRect);
		tmp = tmp.scaled(
			destRect.width(),
			destRect.height(),
			Qt::IgnoreAspectRatio,
			d->mTransformationMode);
	} else {
		// Adjust sourceRect so that the scaling algorithm has enough pixels around
		// to correctly smooth borders
		sourceRect.adjust(-SMOOTH_MARGIN, -SMOOTH_MARGIN, SMOOTH_MARGIN, SMOOTH_MARGIN);

		// Scale the source rect
		tmp = d->mImage.copy(sourceRect);
		int destMargin = int(SMOOTH_MARGIN * d->mZoom);
		tmp = tmp.scaled(
			destRect.width() + 2 * destMargin,
			destRect.height() + 2 * destMargin,
			Qt::IgnoreAspectRatio,
			d->mTransformationMode);

		// Remove the extra pixels added before
		tmp = tmp.copy(destMargin, destMargin, destRect.width(), destRect.height());
	}

	scaledRect(destRect.left(), destRect.top(), tmp);
}

} // namespace
