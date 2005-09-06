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
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kfilemetainfo.h>

// Local
#include "imageutils/orientation.h"
#include "imageutils/jpegcontent.h"

using namespace std;

const char* ORIENT6_FILE="orient6.jpg";
const int ORIENT6_WIDTH=128;  // This size is the size *after* orientation
const int ORIENT6_HEIGHT=256; // has been applied
const char* CUT_FILE="cut.jpg";
const QString ORIENT6_COMMENT="a comment";
const char* ORIENT1_FILE="test_orient1.jpg";
const QString ORIENT1_VFLIP_FILE="test_orient1_vflip.jpg";
const QString ORIENT1_VFLIP_COMMENT="vflip!";
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


typedef QMap<QString,QString> MetaInfoMap;

MetaInfoMap getMetaInfo(const QString& path) {
	KFileMetaInfo fmi(path);
	QStringList list=fmi.supportedKeys();
	QStringList::ConstIterator it=list.begin();
	MetaInfoMap map;
	
	for ( ; it!=list.end(); ++it) {
		KFileMetaInfoItem item=fmi.item(*it);
		map[*it]=item.string();
	}

	return map;
}

void compareMetaInfo(const QString& path1, const QString& path2, const QStringList& ignoredKeys) {
	MetaInfoMap mim1=getMetaInfo(path1);
	MetaInfoMap mim2=getMetaInfo(path2);

	Q_ASSERT(mim1.keys()==mim2.keys());
	QValueList<QString> keys=mim1.keys();
	QValueList<QString>::ConstIterator it=keys.begin();
	for ( ; it!=keys.end(); ++it) {
		QString key=*it;
		if (ignoredKeys.contains(key)) continue;

		if (mim1[key]!=mim2[key]) {
			kdError() << "Meta info differs. Key:" << key << ", V1:" << mim1[key] << ", V2:" << mim2[key] << endl;
		}
	}
}

int main(int argc, char* argv[]) {
	TestEnvironment testEnv;
	bool result;
	KAboutData aboutData("testjpegcontent", "testjpegcontent", "0");
	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication kapplication;

	// Reading info
	ImageUtils::JPEGContent content;
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
	Q_ASSERT(content.orientation() == ImageUtils::NORMAL);

	// transform()
	content.transform(ImageUtils::VFLIP, true, ORIENT1_VFLIP_COMMENT);
	Q_ASSERT(content.comment() == ORIENT1_VFLIP_COMMENT);
	result=content.save(ORIENT1_VFLIP_FILE);
	Q_ASSERT(result);
	
	// Check the other meta info are still here
	QStringList ignoredKeys;
	ignoredKeys << "Orientation" << "Comment";
	compareMetaInfo(ORIENT6_FILE, ORIENT1_VFLIP_FILE, ignoredKeys);

	// Test that loading and manipulating a truncated file does not crash
	result=content.load(CUT_FILE);
	Q_ASSERT(result);
	Q_ASSERT(content.orientation() == 6);
	Q_ASSERT(content.comment() == ORIENT6_COMMENT);
	kdWarning() << "# Next function should output errors about incomplete image" << endl;
	content.transform(ImageUtils::VFLIP);
	kdWarning() << "#" << endl;
}
