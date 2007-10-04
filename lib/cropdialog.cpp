// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "cropdialog.moc"

// Qt
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>

// KDE
#include "klocale.h"

// Local
#include "croptool.h"
#include "imageview.h"
#include "ui_cropdialog.h"

namespace Gwenview {


struct CropDialogPrivate : public Ui_CropDialog {
	QWidget* mWidget;
	AbstractImageViewTool* mPreviousTool;
	QPointer<CropTool> mCropTool;
	bool mUpdatingFromCropTool;
};


CropDialog::CropDialog(QWidget* parent, ImageView* imageView)
: QWidget(parent)
, d(new CropDialogPrivate) {
	d->mUpdatingFromCropTool = false;
	d->mCropTool = new CropTool(imageView);
	d->mPreviousTool = imageView->currentTool();
	imageView->setCurrentTool(d->mCropTool);
	d->mWidget = new QWidget(this);
	d->setupUi(d->mWidget);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(d->mWidget);

	QPushButton* ok = d->buttonBox->button(QDialogButtonBox::Ok);
	Q_ASSERT(ok);
	ok->setText(i18n("Crop"));

	connect(d->mCropTool, SIGNAL(rectUpdated(const QRect&)),
		SLOT(setCropRect(const QRect&)) );

	connect(d->leftSpinBox, SIGNAL(valueChanged(int)),
		SLOT(updateCropToolRect()) );
	connect(d->topSpinBox, SIGNAL(valueChanged(int)),
		SLOT(updateCropToolRect()) );
	connect(d->widthSpinBox, SIGNAL(valueChanged(int)),
		SLOT(updateCropToolRect()) );
	connect(d->heightSpinBox, SIGNAL(valueChanged(int)),
		SLOT(updateCropToolRect()) );

	connect(d->buttonBox, SIGNAL(accepted()),
		SLOT(slotAccepted()) );
	
	connect(d->buttonBox, SIGNAL(rejected()),
		SLOT(slotRejected()) );
}


CropDialog::~CropDialog() {
	// The crop tool is owned by the image view, so it may already have been
	// deleted, for example if we quit while the dialog is still opened.
	if (d->mCropTool) {
		d->mCropTool->imageView()->setCurrentTool(d->mPreviousTool);
	}
	delete d;
}


QRect CropDialog::cropRect() const {
	QRect rect(
		d->leftSpinBox->value(),
		d->topSpinBox->value(),
		d->widthSpinBox->value(),
		d->heightSpinBox->value()
		);
	return rect;
}


void CropDialog::setImageSize(const QSize& size) {
	d->leftSpinBox->setMaximum(size.width());
	d->widthSpinBox->setMaximum(size.width());
	d->topSpinBox->setMaximum(size.height());
	d->heightSpinBox->setMaximum(size.height());

	d->leftSpinBox->setValue(size.width() / 3);
	d->widthSpinBox->setValue(size.width() / 3);
	d->topSpinBox->setValue(size.height() / 3);
	d->heightSpinBox->setValue(size.height() / 3);
}


void CropDialog::setCropRect(const QRect& rect) {
	d->mUpdatingFromCropTool = true;
	d->leftSpinBox->setValue(rect.left());
	d->topSpinBox->setValue(rect.top());
	d->widthSpinBox->setValue(rect.width());
	d->heightSpinBox->setValue(rect.height());
	d->mUpdatingFromCropTool = false;
}


void CropDialog::updateCropToolRect() {
	if (d->mUpdatingFromCropTool) {
		return;
	}
	d->mCropTool->setRect(cropRect());
}


void CropDialog::slotAccepted() {
	emit finished(QDialog::Accepted);
}


void CropDialog::slotRejected() {
	emit finished(QDialog::Rejected);
}
} // namespace
