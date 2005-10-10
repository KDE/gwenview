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
#ifndef CACHE_H
#define CACHE_H

// Qt
#include <qcstring.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qobject.h>
#include <qtimer.h>
#include <qvaluelist.h>

// KDE
#include <kurl.h>

// Local
#include "imageframe.h"
#include "libgwenview_export.h"
class KConfig;

namespace Gwenview {
class LIBGWENVIEW_EXPORT Cache : public QObject {
Q_OBJECT
public:
	static Cache* instance();
	void addImage( const KURL& url, const ImageFrames& frames, const QCString& format, const QDateTime& timestamp );
	void addFile( const KURL& url, const QByteArray& file, const QDateTime& timestamp );
	void addThumbnail( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp );
	QDateTime timestamp( const KURL& url ) const;
	QByteArray file( const KURL& url ) const;
	void getFrames( const KURL& url, ImageFrames& frames, QCString& format ) const;
	QPixmap thumbnail( const KURL& url, QSize& imagesize ) const;
	void setPriorityURL( const KURL& url, bool set );
	void checkThumbnailSize( int size );
	void readConfig(KConfig*,const QString& group);
	void updateAge();
	void ref();
	void deref();
	enum { DEFAULT_MAXSIZE = 16 * 1024 * 1024 }; // 16MiB
private:
	Cache();
	void checkMaxSize();
	struct ImageData {
		ImageData( const KURL& url, const QByteArray& file, const QDateTime& timestamp );
		ImageData( const KURL& url, const QImage& image, const QCString& format, const QDateTime& timestamp );
		ImageData( const KURL& url, const ImageFrames& frames, const QCString& format, const QDateTime& timestamp );
		ImageData( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp );
		void addFile( const QByteArray& file );
		void addImage( const ImageFrames& frames, const QCString& format );
		void addThumbnail( const QPixmap& thumbnail, QSize imagesize );
		long long cost() const;
		int size() const;
		QByteArray file;
		ImageFrames frames;
		QPixmap thumbnail;
		QSize imagesize;
		QCString format;
		QDateTime timestamp;
		mutable int age;
		bool fast_url;
		bool priority;
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
	int mUsageRefcount;
	QTimer mCleanupTimer;
	QValueList< KURL > mPriorityURLs;
private slots:
	void cleanupTimeout();
};

} // namespace
#endif
