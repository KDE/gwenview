// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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

#include "thumbnailsize.h"


ThumbnailSize::ThumbnailSize(Size value) : mValue(value) {}


ThumbnailSize::ThumbnailSize(const QString& str) {
	QString low=str.lower();
	if (low=="small") {
		mValue=Small;
	} else if (low=="large") {
		mValue=Large;
	} else {
		mValue=Med;
	}
}


int ThumbnailSize::pixelSize() const {
	switch (mValue) {
	case Small:
		return 48;
	case Med:
		return 96;
	default: /* Always Large, but keeps the compiler from complaining */
		return 192;
	}
}


ThumbnailSize::operator const QString&() const {
// Little "hack" to allow returning a reference instead of a new QString
	static QString sizeStr[3]={"small","med","large"};
	return sizeStr[int(mValue)];
}
