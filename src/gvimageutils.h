// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
 
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

// Qt
#include <qmemarray.h>
#include <qimage.h>

// KDE
#include <kurl.h>

namespace GVImageUtils {
	enum Orientation { NOT_AVAILABLE=0,NORMAL=1,HFLIP=2,ROT_180=3,VFLIP=4,ROT_90_HFLIP=5,ROT_90=6,ROT_90_VFLIP=7,ROT_270=8};

	enum SmoothAlgorithm { SMOOTH_NONE, SMOOTH_FAST, SMOOTH_NORMAL, SMOOTH_BEST };

	QImage scale(const QImage& image,int width, int height,
		SmoothAlgorithm alg, QImage::ScaleMode mode = QImage::ScaleFree, double blur = 1.0);

	QByteArray setOrientation(const QByteArray& jpegContent, Orientation orientation);
	Orientation getOrientation(const QByteArray& jpegContent);
	Orientation getOrientation(const QString& pixPath);

	QImage modify(const QImage& img, Orientation orientation);

	void getOrientationAndThumbnail(const QString& pixPath, Orientation& orientation, QImage& image);
}

#endif
