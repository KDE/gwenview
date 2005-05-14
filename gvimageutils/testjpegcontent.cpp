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
#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qstring.h>

// KDE
#include <kdebug.h>

// Local
#include "gvimageutils/orientation.h"
#include "gvimageutils/jpegcontent.h"

using namespace std;

const char* ORIENT6_FILE="orient6.jpg";
const int ORIENT6_WIDTH=128;  // This size is the size *after* orientation
const int ORIENT6_HEIGHT=256; // has been applied
const char* CUT_FILE="cut.jpg";
const char* ORIENT6_COMMENT="a comment";
const char* ORIENT1_FILE="test_orient1.jpg";
const char* ORIENT1_VFLIP_FILE="test_orient1_vflip.jpg";
const char* ORIENT1_VFLIP_COMMENT="vflip!";
const char* THUMBNAIL_FILE="test_thumbnail.jpg";


class TestEnvironment {
public:
	TestEnvironment() {
		bool result;
		QFile in(ORIENT6_FILE);
		result=in.open(IO_ReadOnly);
		Q_ASSERT(result);
		
		QFileInfo info(in);
		int size=info.size()/2;
		
		char* data=new char[size];
		int readSize=in.readBlock(data, size);
		Q_ASSERT(size==readSize);
		
		QFile out(CUT_FILE);
		result=out.open(IO_WriteOnly);
		Q_ASSERT(result);
		
		int wroteSize=out.writeBlock(data, size);
		Q_ASSERT(size==wroteSize);
		delete []data;
	}

	~TestEnvironment() {
		QDir::current().remove(CUT_FILE);
	}
};


int main() {
	TestEnvironment testEnv;
	bool result;

	// Reading info
	GVImageUtils::JPEGContent content;
	result=content.load(ORIENT6_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.orientation() == 6);
	Q_ASSERT(content.comment() == ORIENT6_COMMENT);
	Q_ASSERT(content.size() == QSize(ORIENT6_WIDTH, ORIENT6_HEIGHT));
	
	// thumbnail()
	QImage thumbnail=content.thumbnail();
	result=thumbnail.save(THUMBNAIL_FILE, "JPEG");
	Q_ASSERT(result);

	// resetOrientation()
	content.resetOrientation();
	result=content.save(ORIENT1_FILE);
	Q_ASSERT(result);

	result=content.load(ORIENT1_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.orientation() == GVImageUtils::NORMAL);

	// transform()
	content.transform(GVImageUtils::VFLIP, true, ORIENT1_VFLIP_COMMENT);
	Q_ASSERT(content.comment() == ORIENT1_VFLIP_COMMENT);
	result=content.save(ORIENT1_VFLIP_FILE);
	Q_ASSERT(result);
	
	// We load it again because previous call to content.comment() did not read
	// from the JPEG, it used a cached entry instead
	result=content.load(ORIENT1_VFLIP_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.comment() == ORIENT1_VFLIP_COMMENT);

	// Test that loading and manipulating a truncated file does not crash
	result=content.load(CUT_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.orientation() == 6);
	Q_ASSERT(content.comment() == ORIENT6_COMMENT);
	kdWarning() << "# Next function should output errors about incomplete image" << endl;
	content.transform(GVImageUtils::VFLIP);
	kdWarning() << "#" << endl;
}
