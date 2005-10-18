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
#ifndef DOUBLESPINBOX_H
#define DOUBLESPINBOX_H   

// Qt
#include <qspinbox.h>

namespace Gwenview {

class DoubleSpinBox : public QSpinBox {
public:
	DoubleSpinBox(QWidget* parent, const char* name=0);

	static int doubleToInt(double);
	static double intToDouble(int);

protected:
	virtual QString mapValueToText(int);
	virtual int mapTextToValue(bool* ok);
};

} // namespace

#endif /* DOUBLESPINBOX_H */
