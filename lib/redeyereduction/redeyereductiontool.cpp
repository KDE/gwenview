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
#include <QPushButton>
#include <QRect>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "imageview.h"
#include "redeyereductionimageoperation.h"

namespace Gwenview {


struct RedEyeReductionToolPrivate {
	RedEyeReductionTool* mRedEyeReductionTool;
	RedEyeReductionTool::Status mStatus;
	QPoint mCenter;
	int mRadius;
	QPushButton* mApplyButton;

	void updateHud() {
		if (mStatus != RedEyeReductionTool::Adjusting) {
			mApplyButton->hide();
			return;
		}
		const ImageView* view = mRedEyeReductionTool->imageView();
		QPoint pos = view->mapToViewport(mCenter);
		pos.ry() += mRadius * view->zoom() + 5;
		mApplyButton->move(pos);
		mApplyButton->adjustSize();
		mApplyButton->show();
	}
};


RedEyeReductionTool::RedEyeReductionTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new RedEyeReductionToolPrivate) {
	d->mRedEyeReductionTool = this;
	d->mRadius = 12;
	d->mStatus = NotSet;
	d->mApplyButton = new QPushButton(view->viewport());
	d->mApplyButton->setText(i18n("Apply"));
	d->mApplyButton->hide();
	connect(d->mApplyButton, SIGNAL(clicked()), SLOT(slotApplyClicked()));
}


RedEyeReductionTool::~RedEyeReductionTool() {
	delete d;
}


int RedEyeReductionTool::radius() const {
	return d->mRadius;
}


void RedEyeReductionTool::setRadius(int radius) {
	d->mRadius = radius;
	d->updateHud();
	imageView()->viewport()->update();
}


QRect RedEyeReductionTool::rect() const {
	if (d->mStatus == NotSet) {
		return QRect();
	}
	return QRect(d->mCenter.x() - d->mRadius, d->mCenter.y() - d->mRadius, d->mRadius * 2, d->mRadius * 2);
}


void RedEyeReductionTool::paint(QPainter* painter) {
	if (d->mStatus == NotSet) {
		return;
	}
	QRect docRect = rect();
	QImage img = imageView()->document()->image().copy(docRect);
	const QRectF viewRectF = imageView()->mapToViewportF(docRect);

	RedEyeReductionImageOperation::apply(&img, img.rect());
	painter->drawImage(viewRectF, img);
}


void RedEyeReductionTool::mousePressEvent(QMouseEvent* event) {
	d->mCenter = imageView()->mapToImage(event->pos());
	d->mStatus = Adjusting;

	d->updateHud();
	imageView()->viewport()->update();
}


void RedEyeReductionTool::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::NoButton) {
		return;
	}
	d->updateHud();
	d->mCenter = imageView()->mapToImage(event->pos());
	imageView()->viewport()->update();
}


void RedEyeReductionTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::CrossCursor);
}


void RedEyeReductionTool::slotApplyClicked() {
	emit applyClicked();
	d->mStatus = NotSet;
	d->updateHud();
}


} // namespace
