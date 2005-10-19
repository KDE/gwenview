/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau
 
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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Self
#include "doublespinbox.h"

// Qt
#include <qvalidator.h>

// KDE
#include <kglobal.h>
#include <klocale.h>

namespace Gwenview {
static const double DOUBLE_FACTOR=1000.;
static const int INT_FACTOR=int(DOUBLE_FACTOR);
static const int DOUBLE_PRECISION=2; // DOUBLE_FACTOR precision minus one, to hide round errors
	
DoubleSpinBox::DoubleSpinBox(QWidget* parent, const char* name)
: QSpinBox(parent, name) {
	setValidator(new QDoubleValidator(this));
}

int DoubleSpinBox::doubleToInt(double value) {
	return int(value*INT_FACTOR);
}

double DoubleSpinBox::intToDouble(int value) {
	return value/DOUBLE_FACTOR;
}

double DoubleSpinBox::doubleValue() const {
	return intToDouble(value());
}

void DoubleSpinBox::setDoubleValue(double value) {
	setValue(doubleToInt(value));
}

QString DoubleSpinBox::mapValueToText(int value) {
	return KGlobal::locale()->formatNumber(intToDouble(value), DOUBLE_PRECISION);
}

int DoubleSpinBox::mapTextToValue(bool* ok) {
	Q_ASSERT(ok);
	
	// Allow the user to use '.' as a separator, even if it's not the local
	// decimal symbol
	QString txt=text();
	KLocale* locale=KGlobal::locale();
	if (txt.contains(".")==1 && locale->decimalSymbol()!=".") {
		txt=txt.replace(".", locale->decimalSymbol());
	}

	double value=locale->readNumber(txt, ok);
	if (!*ok) return 0;
	return doubleToInt(value);
}

} // namespace
