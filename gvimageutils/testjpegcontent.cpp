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
#include <iostream>

// Qt
#include <qimage.h>
#include <qstring.h>

// KDE
#include <kdebug.h>

// Local
#include "gvimageutils/orientation.h"
#include "gvimageutils/jpegcontent.h"

using namespace std;

const char* ORIENT6_FILE="orient6.jpg";
const char* ORIENT1_FILE="test_orient1.jpg";
const char* ORIENT1_VFLIP_FILE="test_orient1_vflip.jpg";
const char* THUMBNAIL_FILE="test_thumbnail.jpg";

int main() {
	bool result;

	GVImageUtils::JPEGContent content;
	result=content.load(ORIENT6_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.orientation() == 6);
	
	QImage thumbnail=content.thumbnail();
	result=thumbnail.save(THUMBNAIL_FILE, "JPEG");
	Q_ASSERT(result);

	content.resetOrientation();
	result=content.save(ORIENT1_FILE);
	Q_ASSERT(result);

	result=content.load(ORIENT1_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.orientation() == GVImageUtils::NORMAL);

	content.transform(GVImageUtils::VFLIP);
	result=content.save(ORIENT1_VFLIP_FILE);
	Q_ASSERT(result);
}
