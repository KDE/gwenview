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
	QButtonGroup mScaleGroup;
};


PrintOptionsPage::PrintOptionsPage()
: QWidget()
, d(new PrintOptionsPagePrivate) {
	d->setupUi(this);
	d->mScaleGroup.addButton(d->mNoScale, NoScale);
	d->mScaleGroup.addButton(d->mScaleToPage, ScaleToPage);
	d->mScaleGroup.addButton(d->mScaleTo, ScaleToCustomSize);
}


PrintOptionsPage::~PrintOptionsPage() {
	delete d;
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


void PrintOptionsPage::loadConfig() {
}


void PrintOptionsPage::saveConfig() {
}


} // namespace
