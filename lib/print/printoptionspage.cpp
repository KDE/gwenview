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
#include "printoptionspage.moc"

// Qt
#include <QButtonGroup>

// KDE
#include <kconfigdialogmanager.h>

// Local
#include <gwenviewconfig.h>
#include <signalblocker.h>
#include <ui_printoptionspage.h>

namespace Gwenview {


static inline double unitToInches(PrintOptionsPage::Unit unit) {
	if (unit == PrintOptionsPage::Inches) {
		return 1.;
	} else if (unit == PrintOptionsPage::Centimeters) {
		return 1/2.54;
	} else { // Millimeters
		return 1/25.4;
	}
}


struct PrintOptionsPagePrivate : public Ui_PrintOptionsPage {
	QSize mImageSize;
	QButtonGroup mScaleGroup;
	KConfigDialogManager* mConfigDialogManager;
};


PrintOptionsPage::PrintOptionsPage(const QSize& imageSize)
: QWidget()
, d(new PrintOptionsPagePrivate) {
	d->setupUi(this);
	d->mImageSize = imageSize;
	d->mConfigDialogManager = new KConfigDialogManager(this, GwenviewConfig::self());

	d->mPosition->setItemData(0, int(Qt::AlignTop     | Qt::AlignLeft));
	d->mPosition->setItemData(1, int(Qt::AlignTop     | Qt::AlignHCenter));
	d->mPosition->setItemData(2, int(Qt::AlignTop     | Qt::AlignRight));
	d->mPosition->setItemData(3, int(Qt::AlignVCenter | Qt::AlignLeft));
	d->mPosition->setItemData(4, int(Qt::AlignVCenter | Qt::AlignHCenter));
	d->mPosition->setItemData(5, int(Qt::AlignVCenter | Qt::AlignRight));
	d->mPosition->setItemData(6, int(Qt::AlignBottom  | Qt::AlignLeft));
	d->mPosition->setItemData(7, int(Qt::AlignBottom  | Qt::AlignHCenter));
	d->mPosition->setItemData(8, int(Qt::AlignBottom  | Qt::AlignRight));

	d->mScaleGroup.addButton(d->mNoScale, NoScale);
	d->mScaleGroup.addButton(d->mScaleToPage, ScaleToPage);
	d->mScaleGroup.addButton(d->mScaleTo, ScaleToCustomSize);

	connect(d->kcfg_PrintWidth, SIGNAL(valueChanged(double)),
		SLOT(adjustHeightToRatio()) );

	connect(d->kcfg_PrintHeight, SIGNAL(valueChanged(double)),
		SLOT(adjustWidthToRatio()) );

	connect(d->kcfg_PrintKeepRatio, SIGNAL(toggled(bool)),
		SLOT(adjustHeightToRatio()) );
}


PrintOptionsPage::~PrintOptionsPage() {
	delete d;
}


Qt::Alignment PrintOptionsPage::alignment() const {
	QVariant data = d->mPosition->itemData(d->mPosition->currentIndex());
	return Qt::Alignment(data.toInt());
}


PrintOptionsPage::ScaleMode PrintOptionsPage::scaleMode() const {
	return PrintOptionsPage::ScaleMode( d->mScaleGroup.checkedId() );
}


bool PrintOptionsPage::enlargeSmallerImages() const {
	return d->kcfg_PrintEnlargeSmallerImages->isChecked();
}


PrintOptionsPage::Unit PrintOptionsPage::scaleUnit() const {
	return PrintOptionsPage::Unit(d->kcfg_PrintUnit->currentIndex());
}


double PrintOptionsPage::scaleWidth() const {
	return d->kcfg_PrintWidth->value() * unitToInches(scaleUnit());
}


double PrintOptionsPage::scaleHeight() const {
	return d->kcfg_PrintHeight->value() * unitToInches(scaleUnit());
}


void PrintOptionsPage::adjustWidthToRatio() {
	if (!d->kcfg_PrintKeepRatio->isChecked()) {
		return;
	}
	double width = d->mImageSize.width() * d->kcfg_PrintHeight->value() / d->mImageSize.height();

	SignalBlocker blocker(d->kcfg_PrintWidth);
	d->kcfg_PrintWidth->setValue(width ? width : 1.);
}


void PrintOptionsPage::adjustHeightToRatio() {
	if (!d->kcfg_PrintKeepRatio->isChecked()) {
		return;
	}
	double height = d->mImageSize.height() * d->kcfg_PrintWidth->value() / d->mImageSize.width();

	SignalBlocker blocker(d->kcfg_PrintHeight);
	d->kcfg_PrintHeight->setValue(height ? height : 1.);
}


void PrintOptionsPage::loadConfig() {
	int position = GwenviewConfig::printPosition();
	int index = d->mPosition->findData(position);
	if (index != -1) {
		d->mPosition->setCurrentIndex(index);
	}

	QAbstractButton* button = d->mScaleGroup.button(GwenviewConfig::printScaleMode());
	if (button) {
		button->setChecked(true);
	}

	d->mConfigDialogManager->updateWidgets();

	if (d->kcfg_PrintKeepRatio->isChecked()) {
		adjustHeightToRatio();
	}
}


void PrintOptionsPage::saveConfig() {
	QVariant data = d->mPosition->itemData(d->mPosition->currentIndex());
	GwenviewConfig::setPrintPosition(data.toInt());

	ScaleMode scaleMode = ScaleMode( d->mScaleGroup.checkedId() );
	GwenviewConfig::setPrintScaleMode(scaleMode);

	d->mConfigDialogManager->updateSettings();

	GwenviewConfig::self()->writeConfig();
}


} // namespace
