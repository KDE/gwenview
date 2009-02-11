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
#include <QStyle>
#include <QToolButton>
#include <QRect>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include <lib/hudwidget.h>
#include "gwenviewconfig.h"
#include "imageview.h"
#include "paintutils.h"
#include "redeyereductionimageoperation.h"
#include "ui_redeyereductionhud.h"


namespace Gwenview {


struct RedEyeReductionHud : public QWidget, public Ui_RedEyeReductionHud {
	RedEyeReductionHud() {
		setupUi(this);
		setCursor(Qt::ArrowCursor);
	}
};


struct RedEyeReductionToolPrivate {
	RedEyeReductionTool* mRedEyeReductionTool;
	RedEyeReductionTool::Status mStatus;
	QPointF mCenter;
	int mDiameter;
	RedEyeReductionHud* mHud;
	HudWidget* mHudWidget;


	void showNotSetHudWidget() {
		mHud->deleteLater();
		mHud = 0;
		QLabel* label = new QLabel(i18n("Click on the red eye you want to fix."));
		label->show();
		label->adjustSize();
		createHudWidgetForWidget(label);
	}


	void showAdjustingHudWidget() {
		mHud = new RedEyeReductionHud();

		mHud->diameterSpinBox->setValue(mDiameter);
		QObject::connect(mHud->applyButton, SIGNAL(clicked()),
			mRedEyeReductionTool, SLOT(slotApplyClicked()));
		QObject::connect(mHud->diameterSpinBox, SIGNAL(valueChanged(int)),
			mRedEyeReductionTool, SLOT(setDiameter(int)));

		createHudWidgetForWidget(mHud);
	}


	void createHudWidgetForWidget(QWidget* widget) {
		QPoint pos;
		if (mHudWidget) {
			pos = mHudWidget->pos();
			mHudWidget->deleteLater();
		} else {
			pos = QPoint(KDialog::marginHint(), KDialog::marginHint());
		}
		mHudWidget = new HudWidget(mRedEyeReductionTool->imageView());
		mHudWidget->init(widget, HudWidget::OptionCloseButton | HudWidget::OptionDragHandle);
		mHudWidget->adjustSize();
		QObject::connect(mHudWidget->closeButton(), SIGNAL(clicked()),
			mRedEyeReductionTool, SIGNAL(done()) );
		mHudWidget->move(pos);
		mHudWidget->show();
	}


	void hideHud() {
		mHudWidget->hide();
	}


	QRectF rectF() const {
		if (mStatus == RedEyeReductionTool::NotSet) {
			return QRectF();
		}
		return QRectF(mCenter.x() - mDiameter / 2, mCenter.y() - mDiameter / 2, mDiameter, mDiameter);
	}
};


RedEyeReductionTool::RedEyeReductionTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new RedEyeReductionToolPrivate) {
	d->mRedEyeReductionTool = this;
	d->mDiameter = GwenviewConfig::redEyeReductionDiameter();
	d->mStatus = NotSet;
	d->mHud = 0;
	d->mHudWidget = 0;

	d->showNotSetHudWidget();

	view->document()->loadFullImage();
}


RedEyeReductionTool::~RedEyeReductionTool() {
	GwenviewConfig::setRedEyeReductionDiameter(d->mDiameter);
	delete d;
}


void RedEyeReductionTool::paint(QPainter* painter) {
	if (d->mStatus == NotSet) {
		return;
	}
	QRectF docRectF = d->rectF();
	imageView()->document()->waitUntilLoaded();

	QRect docRect = PaintUtils::containingRect(docRectF);
	QImage img = imageView()->document()->image().copy(docRect);
	QRectF imgRectF(
		docRectF.left() - docRect.left(),
		docRectF.top()  - docRect.top(),
		docRectF.width(),
		docRectF.height()
		);
	RedEyeReductionImageOperation::apply(&img, imgRectF);

	const QRectF viewRectF = imageView()->mapToViewportF(docRectF);
	painter->drawImage(viewRectF, img, imgRectF);
}


void RedEyeReductionTool::mousePressEvent(QMouseEvent* event) {
	if (d->mStatus == NotSet) {
		d->showAdjustingHudWidget();
		d->mStatus = Adjusting;
	}
	d->mCenter = imageView()->mapToImageF(event->pos());
	imageView()->viewport()->update();
}


void RedEyeReductionTool::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::NoButton) {
		return;
	}
	d->mCenter = imageView()->mapToImageF(event->pos());
	imageView()->viewport()->update();
}


void RedEyeReductionTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::CrossCursor);
}


void RedEyeReductionTool::toolDeactivated() {
	d->mHudWidget->deleteLater();
}


void RedEyeReductionTool::slotApplyClicked() {
	QRectF docRectF = d->rectF();
	if (!docRectF.isValid()) {
		kWarning() << "invalid rect";
		return;
	}
	RedEyeReductionImageOperation* op = new RedEyeReductionImageOperation(docRectF);
	emit imageOperationRequested(op);

	d->mStatus = NotSet;
	d->showNotSetHudWidget();
}


void RedEyeReductionTool::setDiameter(int value) {
	d->mDiameter = value;
	imageView()->viewport()->update();
}


} // namespace
