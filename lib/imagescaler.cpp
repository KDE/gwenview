/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QMutex>
#include <QMutexLocker>
#include <QRegion>
#include <QTimer>

// KDE
#include <kdebug.h>

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

namespace Gwenview {

// Amount of pixels to keep so that smooth scale is correct
static const int SMOOTH_MARGIN = 3;

static const int MAX_CHUNK_AREA = 1000 * 100;

static const int MAX_SCALE_TIME = 2000;

struct ImageScalerPrivate {
	Qt::TransformationMode mTransformationMode;
	const QImage* mImage;
	qreal mZoom;
	QRegion mRegion;

	QMutex mMutex;
	bool mStopRequested;

	void stop() {
		QMutexLocker locker(&mMutex);
		mStopRequested = true;
	}
};

ImageScaler::ImageScaler(QObject* parent)
: QThread(parent)
, d(new ImageScalerPrivate) {
	d->mTransformationMode = Qt::FastTransformation;
	d->mStopRequested = false;
}

ImageScaler::~ImageScaler() {
	delete d;
}

void ImageScaler::setImage(const QImage* image) {
	d->stop();
	wait();
	d->mImage = image;
}

void ImageScaler::setZoom(qreal zoom) {
	d->stop();
	wait();
	d->mZoom = zoom;
}

void ImageScaler::setTransformationMode(Qt::TransformationMode mode) {
	d->stop();
	wait();
	d->mTransformationMode = mode;
}

void ImageScaler::setDestinationRegion(const QRegion& region) {
	LOG(region);
	d->stop();
	wait();
	d->mRegion = region;
	if (d->mRegion.isEmpty()) {
		return;
	}

	if (d->mImage && !d->mImage->isNull()) {
		start();
	}
}

void ImageScaler::addDestinationRegion(const QRegion& region) {
	LOG(region);
	{
		QMutexLocker locker(&d->mMutex);
		d->mRegion |= region;
		if (d->mRegion.isEmpty()) {
			return;
		}
	}

	if (d->mImage && !d->mImage->isNull() && !isRunning()) {
		start();
	}
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


void ImageScaler::run() {
	LOG("Starting");
	{
		QMutexLocker locker(&d->mMutex);
		d->mStopRequested = false;
	}

	while (true) {
		QRect rect;
		// Extract a rect to scale from d->mRegion
		{
			QMutexLocker locker(&d->mMutex);
			if (d->mStopRequested) {
				LOG("Stopped");
				break;
			}

			rect = d->mRegion.rects()[0];
			if (rect.width() * rect.height() > MAX_CHUNK_AREA) {
				int height = qMax(1, MAX_CHUNK_AREA / rect.width());
				rect.setHeight(height);
			}
		}

		LOG(rect);
		processChunk(rect);

		{
			QMutexLocker locker(&d->mMutex);
			d->mRegion -= rect;
			if (d->mRegion.isEmpty()) {
				break;
			}
		}
	}
	LOG("Done");
}


void ImageScaler::processChunk(const QRect& rect) {
	if (qAbs(d->mZoom - 1.0) < 0.001) {
		QImage tmp = d->mImage->copy(rect);
		scaledRect(rect.left(), rect.top(), tmp);
		return;
	}
	// If rect contains "half" pixels, make sure sourceRect includes them
	QRectF sourceRectF(
		rect.left() / d->mZoom,
		rect.top() / d->mZoom,
		rect.width() / d->mZoom,
		rect.height() / d->mZoom);

	sourceRectF = sourceRectF.intersected(d->mImage->rect());
	QRect sourceRect = containingRect(sourceRectF);
	if (sourceRect.isEmpty()) {
		return;
	}

	// Compute smooth margin
	bool needsSmoothMargins = d->mTransformationMode == Qt::SmoothTransformation;

	int sourceLeftMargin, sourceRightMargin, sourceTopMargin, sourceBottomMargin;
	int destLeftMargin, destRightMargin, destTopMargin, destBottomMargin;
	if (needsSmoothMargins) {
		sourceLeftMargin = qMin(sourceRect.left(), SMOOTH_MARGIN);
		sourceTopMargin = qMin(sourceRect.top(), SMOOTH_MARGIN);
		sourceRightMargin = qMin(d->mImage->rect().right() - sourceRect.right(), SMOOTH_MARGIN);
		sourceBottomMargin = qMin(d->mImage->rect().bottom() - sourceRect.bottom(), SMOOTH_MARGIN);
		sourceRect.adjust(
			-sourceLeftMargin,
			-sourceTopMargin,
			sourceRightMargin,
			sourceBottomMargin);
		destLeftMargin = int(sourceLeftMargin * d->mZoom);
		destTopMargin = int(sourceTopMargin * d->mZoom);
		destRightMargin = int(sourceRightMargin * d->mZoom);
		destBottomMargin = int(sourceBottomMargin * d->mZoom);
	} else {
		sourceLeftMargin = sourceRightMargin = sourceTopMargin = sourceBottomMargin = 0;
		destLeftMargin = destRightMargin = destTopMargin = destBottomMargin = 0;
	}

	// destRect is almost like rect, but it contains only "full" pixels
	QRectF destRectF = QRectF(
		sourceRect.left() * d->mZoom,
		sourceRect.top() * d->mZoom,
		sourceRect.width() * d->mZoom,
		sourceRect.height() * d->mZoom
		);
	QRect destRect = containingRect(destRectF);

	QImage tmp;
	tmp = d->mImage->copy(sourceRect);
	tmp = tmp.scaled(
		destRect.width(),
		destRect.height(),
		Qt::KeepAspectRatio,
		d->mTransformationMode);

	if (needsSmoothMargins) {
		tmp = tmp.copy(
			destLeftMargin, destTopMargin,
			destRect.width() - (destLeftMargin + destRightMargin),
			destRect.height() - (destTopMargin + destBottomMargin)
			);
	}

	scaledRect(destRect.left() + destLeftMargin, destRect.top() + destTopMargin, tmp);
}

} // namespace
