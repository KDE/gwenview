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
#include "cropdialog.h"

// Qt
#include <QSpinBox>

// KDE
#include "klocale.h"

// Local
#include "ui_cropdialog.h"

namespace Gwenview {


struct CropDialogPrivate : public Ui_CropDialog {
	QWidget* mWidget;
};


CropDialog::CropDialog(QWidget* parent)
: KDialog(parent)
, d(new CropDialogPrivate) {
	d->mWidget = new QWidget(this);
	setMainWidget(d->mWidget);
	d->setupUi(d->mWidget);

	setCaption(i18n("Crop Image"));
	setButtons(KDialog::Ok | KDialog::Cancel);
	setButtonText(KDialog::Ok, i18n("&Crop"));
	showButtonSeparator(true);
}


CropDialog::~CropDialog() {
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
	d->widthSpinBox->setValue(size.width());
	d->topSpinBox->setMaximum(size.height());
	d->heightSpinBox->setMaximum(size.height());
	d->heightSpinBox->setValue(size.height());
}


} // namespace
