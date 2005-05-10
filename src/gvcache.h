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
#ifndef GVCACHE_H
#define GVCACHE_H

// Qt
#include <qcstring.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qobject.h>

// KDE
#include <kurl.h>

// Local
#include "gvimageframe.h"
#include "libgwenview_export.h"
class KConfig;

class LIBGWENVIEW_EXPORT GVCache {
public:
	static GVCache* instance();
	void addImage( const KURL& url, const GVImageFrames& frames, const QCString& format, const QDateTime& timestamp );
	void addFile( const KURL& url, const QByteArray& file, const QDateTime& timestamp );
	void addThumbnail( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp );
	QDateTime timestamp( const KURL& url ) const;
	QByteArray file( const KURL& url ) const;
	void getFrames( const KURL& url, GVImageFrames& frames, QCString& format ) const;
	QPixmap thumbnail( const KURL& url, QSize& imagesize ) const;
	void checkThumbnailSize( int size );
	void readConfig(KConfig*,const QString& group);
	void updateAge();
	enum { DEFAULT_MAXSIZE = 16 * 1024 * 1024 }; // 16MiB
private:
	GVCache();
	void checkMaxSize();
	struct ImageData {
		ImageData( const KURL& url, const QByteArray& file, const QDateTime& timestamp );
		ImageData( const KURL& url, const QImage& image, const QCString& format, const QDateTime& timestamp );
		ImageData( const KURL& url, const GVImageFrames& frames, const QCString& format, const QDateTime& timestamp );
		ImageData( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp );
		void addFile( const QByteArray& file );
		void addImage( const GVImageFrames& frames, const QCString& format );
		void addThumbnail( const QPixmap& thumbnail, QSize imagesize );
		long long cost() const;
		int size() const;
		QByteArray file;
		GVImageFrames frames;
		QPixmap thumbnail;
		QSize imagesize;
		QCString format;
		QDateTime timestamp;
		mutable int age;
		bool fast_url;
		int fileSize() const;
		int imageSize() const;
		int thumbnailSize() const;
		bool reduceSize();
		bool isEmpty() const;
		ImageData() {}; // stupid QMap
	};
	QMap< KURL, ImageData > mImages;
	int mMaxSize;
	int mThumbnailSize;
};

#endif
