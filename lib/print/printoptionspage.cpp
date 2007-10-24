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

// Local
#include <ui_printoptionspage.h>
#include <gwenviewconfig.h>

namespace Gwenview {


class SignalBlocker {
public:
	SignalBlocker(QObject* object) {
		mObject = object;
		mWasBlocked = object->blockSignals(true);
	}

	~SignalBlocker() {
		mObject->blockSignals(mWasBlocked);
	}

private:
	QObject* mObject;
	bool mWasBlocked;
};


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
};


PrintOptionsPage::PrintOptionsPage(const QSize& imageSize)
: QWidget()
, d(new PrintOptionsPagePrivate) {
	d->setupUi(this);
	d->mImageSize = imageSize;

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

	connect(d->mWidth, SIGNAL(valueChanged(double)),
		SLOT(adjustHeightToRatio()) );

	connect(d->mHeight, SIGNAL(valueChanged(double)),
		SLOT(adjustWidthToRatio()) );

	connect(d->mKeepRatio, SIGNAL(toggled(bool)),
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
	return d->mEnlargeSmallerImages->isChecked();
}


PrintOptionsPage::Unit PrintOptionsPage::scaleUnit() const {
	return PrintOptionsPage::Unit(d->mUnit->currentIndex());
}


double PrintOptionsPage::scaleWidth() const {
	return d->mWidth->value() * unitToInches(scaleUnit());
}


double PrintOptionsPage::scaleHeight() const {
	return d->mHeight->value() * unitToInches(scaleUnit());
}


void PrintOptionsPage::adjustWidthToRatio() {
	if (!d->mKeepRatio->isChecked()) {
		return;
	}
	double width = d->mImageSize.width() * d->mHeight->value() / d->mImageSize.height();

	SignalBlocker blocker(d->mWidth);
	d->mWidth->setValue(width ? width : 1.);
}


void PrintOptionsPage::adjustHeightToRatio() {
	if (!d->mKeepRatio->isChecked()) {
		return;
	}
	double height = d->mImageSize.height() * d->mWidth->value() / d->mImageSize.width();

	SignalBlocker blocker(d->mHeight);
	d->mHeight->setValue(height ? height : 1.);
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

	if (d->mKeepRatio->isChecked()) {
		adjustHeightToRatio();
	}
}


void PrintOptionsPage::saveConfig() {
	QVariant data = d->mPosition->itemData(d->mPosition->currentIndex());
	GwenviewConfig::setPrintPosition(data.toInt());

	ScaleMode scaleMode = ScaleMode( d->mScaleGroup.checkedId() );
	GwenviewConfig::setPrintScaleMode(scaleMode);

	GwenviewConfig::self()->writeConfig();
}


} // namespace
