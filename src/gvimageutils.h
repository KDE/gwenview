/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau
 
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
#ifndef GVIMAGEUTILS_H
#define GVIMAGEUTILS_H

template <class>class QMemArray;
typedef QMemArray<char> QByteArray;
class QImage;

namespace GVImageUtils {
	enum Orientation { NotAvailable=0,Normal=1,HFlip=2,Rot180=3,VFlip=4,Rot90HFlip=5,Rot90=6,Rot90VFlip=7,Rot270=8};

	QByteArray setOrientation(const QByteArray& jpegContent, Orientation orientation);
	Orientation getOrientation(const QByteArray& jpegContent);
	Orientation getOrientation(const QString& pixPath);

	QImage rotate(const QImage& img, Orientation orientation);
}

#endif
