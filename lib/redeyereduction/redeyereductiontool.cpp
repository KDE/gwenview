// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "redeyereductiontool.moc"

// Qt
#include <QMouseEvent>
#include <QPainter>
#include <QRect>

// KDE
#include <kdebug.h>

// Local
#include "imageview.h"
#include "redeyereductionimageoperation.h"

static const int UNINITIALIZED_X = -1;

namespace Gwenview {


struct RedEyeReductionToolPrivate {
	RedEyeReductionTool* mRedEyeReductionTool;
	QPoint mCenter;
	int mRadius;
};


RedEyeReductionTool::RedEyeReductionTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new RedEyeReductionToolPrivate) {
	d->mRedEyeReductionTool = this;
	d->mCenter.setX(UNINITIALIZED_X);
	d->mRadius = 12;
}


RedEyeReductionTool::~RedEyeReductionTool() {
	delete d;
}


int RedEyeReductionTool::radius() const {
	return d->mRadius;
}


void RedEyeReductionTool::setRadius(int radius) {
	d->mRadius = radius;
	imageView()->viewport()->update();
}


QRect RedEyeReductionTool::rect() const {
	return QRect(d->mCenter.x() - d->mRadius, d->mCenter.y() - d->mRadius, d->mRadius * 2, d->mRadius * 2);
}


void RedEyeReductionTool::paint(QPainter* painter) {
	if (d->mCenter.x() == UNINITIALIZED_X) {
		return;
	}
	const QRect viewRect = imageView()->mapToViewport(rect());
	QImage img = imageView()->buffer().copy(viewRect).toImage();

	RedEyeReductionImageOperation::apply(&img, img.rect());
	painter->drawImage(viewRect.topLeft(), img);
}


void RedEyeReductionTool::mousePressEvent(QMouseEvent* event) {
	d->mCenter = imageView()->mapToImage(event->pos());
	imageView()->viewport()->update();
}


void RedEyeReductionTool::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::NoButton) {
		return;
	}
	d->mCenter = imageView()->mapToImage(event->pos());
	imageView()->viewport()->update();
}


void RedEyeReductionTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::CrossCursor);
}


} // namespace
