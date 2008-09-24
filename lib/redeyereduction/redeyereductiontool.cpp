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
#include "ui_redeyereductionhud.h"
#include "widgetfloater.h"


namespace Gwenview {

static const int CIRCLE_HUD_SPACING = 20;

class HudWidget : public QFrame {
public:
	HudWidget(QWidget* parent = 0)
	: QFrame(parent) {
		mCloseButton = new QPushButton(this);
		mCloseButton->setIcon(SmallIcon("window-close"));
	}

	void setMainWidget(QWidget* widget) {
		mMainWidget = widget;
		mMainWidget->setParent(this);
		if (mMainWidget->layout()) {
			mMainWidget->layout()->setMargin(0);
		}

		setStyleSheet(
			".QFrame {"
			"	background-color: rgba(10, 10, 10, 80%);"
			"	border: 1px solid rgba(0, 0, 0, 80%);"
			"	border-radius: 8px;"
			"}"
			"QLabel {"
			"	color: white;"
			"}"
			);

		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->setMargin(4);
		layout->addWidget(mMainWidget);
		layout->addWidget(mCloseButton);
	}

	QPushButton* closeButton() const {
		return mCloseButton;
	}

	QWidget* mainWidget() const {
		return mMainWidget;
	}

private:
	QWidget* mMainWidget;
	QPushButton* mCloseButton;
};


struct RedEyeReductionHud : public QWidget, public Ui_RedEyeReductionHud {
	RedEyeReductionHud() {
		setupUi(this);
		setCursor(Qt::ArrowCursor);
	}
};


struct RedEyeReductionToolPrivate {
	RedEyeReductionTool* mRedEyeReductionTool;
	RedEyeReductionTool::Status mStatus;
	QPoint mCenter;
	int mRadius;
	RedEyeReductionHud* mHud;
	HudWidget* mHudWidget;
	WidgetFloater* mFloater;
	QPushButton* mApplyButton;


	void showNotSetHudWidget() {
		delete mHud;
		mHud = 0;
		QLabel* label = new QLabel(i18n("Click on the red eye"));
		label->show();
		label->adjustSize();
		createHudWidgetForWidget(label);
	}


	void showAdjustingHudWidget() {
		mHud = new RedEyeReductionHud();

		mHud->radiusSpinBox->setValue(mRadius);
		QObject::connect(mHud->applyButton, SIGNAL(clicked()),
			mRedEyeReductionTool, SLOT(slotApplyClicked()));
		QObject::connect(mHud->radiusSpinBox, SIGNAL(valueChanged(int)),
			mRedEyeReductionTool, SLOT(setRadius(int)));

		createHudWidgetForWidget(mHud);
	}


	void createHudWidgetForWidget(QWidget* widget) {
		delete mHudWidget;
		mHudWidget = new HudWidget();
		mHudWidget->setMainWidget(widget);
		mHudWidget->adjustSize();
		QObject::connect(mHudWidget->closeButton(), SIGNAL(clicked()),
			mRedEyeReductionTool, SIGNAL(done()) );
		mFloater->setChildWidget(mHudWidget);
	}


	void hideHud() {
		mHudWidget->hide();
	}


	QRect rect() const {
		if (mStatus == RedEyeReductionTool::NotSet) {
			return QRect();
		}
		return QRect(mCenter.x() - mRadius, mCenter.y() - mRadius, mRadius * 2, mRadius * 2);
	}
};


RedEyeReductionTool::RedEyeReductionTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new RedEyeReductionToolPrivate) {
	d->mRedEyeReductionTool = this;
	d->mRadius = 12;
	d->mStatus = NotSet;
	d->mHud = 0;
	d->mHudWidget = 0;

	d->mFloater = new WidgetFloater(imageView());
	d->mFloater->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	d->showNotSetHudWidget();

	view->document()->loadFullImage();
}


RedEyeReductionTool::~RedEyeReductionTool() {
	delete d;
}


void RedEyeReductionTool::paint(QPainter* painter) {
	if (d->mStatus == NotSet) {
		return;
	}
	QRect docRect = d->rect();
	imageView()->document()->waitUntilLoaded();
	QImage img = imageView()->document()->image().copy(docRect);
	const QRectF viewRectF = imageView()->mapToViewportF(docRect);

	RedEyeReductionImageOperation::apply(&img, img.rect());
	painter->drawImage(viewRectF, img);
}


void RedEyeReductionTool::mousePressEvent(QMouseEvent* event) {
	if (d->mStatus == NotSet) {
		d->showAdjustingHudWidget();
		d->mStatus = Adjusting;
	}
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


void RedEyeReductionTool::toolDeactivated() {
	delete d->mHudWidget;
}


void RedEyeReductionTool::slotApplyClicked() {
	QRect docRect = d->rect();
	if (!docRect.isValid()) {
		kWarning() << "invalid rect";
		return;
	}
	RedEyeReductionImageOperation* op = new RedEyeReductionImageOperation(docRect);
	emit imageOperationRequested(op);

	d->mStatus = NotSet;
	d->showNotSetHudWidget();
}


void RedEyeReductionTool::setRadius(int radius) {
	d->mRadius = radius;
	imageView()->viewport()->update();
}


} // namespace
