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

// Qt
#include <QImage>
#include <QRegion>
#include <QTimer>

// KDE
#include <kdebug.h>

namespace Gwenview {

struct ImageScalerPrivate {
	QImage mImage;
	qreal mZoom;
	QRegion mRegion;
	QTimer* mTimer;
};

ImageScaler::ImageScaler(QObject* parent)
: QObject(parent)
, d(new ImageScalerPrivate) {
	d->mTimer = new QTimer(this);
	connect(d->mTimer, SIGNAL(timeout()),
		SLOT(processChunk()) );
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

void ImageScaler::setRegion(const QRegion& region) {
	d->mRegion = region;
}

void ImageScaler::addRegion(const QRegion& region) {
	d->mRegion |= region;
}

void ImageScaler::start() const {
	d->mTimer->start();	
}

bool ImageScaler::isRunning() {
	return d->mTimer->isActive();
}

void ImageScaler::processChunk() {
	Q_ASSERT(!d->mRegion.isEmpty());

	QRect rect = d->mRegion.rects()[0];
	d->mRegion -= rect;
	if (d->mRegion.isEmpty()) {
		d->mTimer->stop();
	}

	QImage tmp;
	tmp = d->mImage.copy(
		int(rect.left() / d->mZoom),
		int(rect.top() / d->mZoom), 
		int(rect.width() / d->mZoom),
		int(rect.height() / d->mZoom)
		);
	tmp = tmp.scaled(rect.size());
	scaledRect(rect.left(), rect.top(), tmp);
}

} // namespace
