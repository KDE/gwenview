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
// Qt
#include <qimage.h>
#include <qwmatrix.h>

// KDE
#include <kdebug.h>

// Local
#include "gvimageutils/orientation.h"

namespace GVImageUtils {

QImage transform(const QImage& img, Orientation orientation) {
	QWMatrix matrix;
	switch (orientation) {
	case NOT_AVAILABLE:
	case NORMAL:
		return img;

	case HFLIP:
		matrix.scale(-1,1);
		break;

	case ROT_180:
		matrix.rotate(180);
		break;

	case VFLIP:
		matrix.scale(1,-1);
		break;
	
	case ROT_90_HFLIP:
		matrix.scale(-1,1);
		matrix.rotate(90);
		break;
		
	case ROT_90:		
		matrix.rotate(90);
		break;
	
	case ROT_90_VFLIP:
		matrix.scale(1,-1);
		matrix.rotate(90);
		break;
		
	case ROT_270:		
		matrix.rotate(270);
		break;
	}

	return img.xForm(matrix);
}


} // Namespace

