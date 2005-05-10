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

#include "gvcache.h"

// Qt

// KDE
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kio/global.h>

// Local
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

//#define DEBUG_CACHE

const char CONFIG_CACHE_MAXSIZE[]="maxSize";

GVCache::GVCache()
: mMaxSize( DEFAULT_MAXSIZE )
, mThumbnailSize( 0 ) // don't remember size for every thumbnail, but have one global and dump all if needed
{
}

GVCache* GVCache::instance() {
	static GVCache manager;
	return &manager;
}

void GVCache::addFile( const KURL& url, const QByteArray& file, const QDateTime& timestamp) {
	LOG(url.prettyURL());
	updateAge();
	bool insert = true;
	if( mImages.contains( url )) {
		ImageData& data = mImages[ url ];
		if( data.timestamp == timestamp ) {
			data.addFile( file );
			insert = false;
		}
	}
	if( insert ) mImages[ url ] = ImageData( url, file, timestamp );
	checkMaxSize();
}

void GVCache::addImage( const KURL& url, const GVImageFrames& frames, const QCString& format, const QDateTime& timestamp ) {
	LOG(url.prettyURL());
	updateAge();
	bool insert = true;
	if( mImages.contains( url )) {
		ImageData& data = mImages[ url ];
		if( data.timestamp == timestamp ) {
			data.addImage( frames, format );
			insert = false;
		}
	}
	if( insert ) mImages[ url ] = ImageData( url, frames, format, timestamp );
	checkMaxSize();
}

void GVCache::addThumbnail( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp ) {
// Thumbnails are many and often - things would age too quickly. Therefore
// when adding thumbnails updateAge() is called from the outside only once for all of them.
//	updateAge();
	bool insert = true;
	if( mImages.contains( url )) {
		ImageData& data = mImages[ url ];
		if( data.timestamp == timestamp ) {
			data.addThumbnail( thumbnail, imagesize );
			insert = false;
		}
	}
	if( insert ) mImages[ url ] = ImageData( url, thumbnail, imagesize, timestamp );
	checkMaxSize();
}

QDateTime GVCache::timestamp( const KURL& url ) const {
	LOG(url.prettyURL());
	if( mImages.contains( url )) return mImages[ url ].timestamp;
	return QDateTime();
}

QByteArray GVCache::file( const KURL& url ) const {
	LOG(url.prettyURL());
	if( mImages.contains( url )) {
		const ImageData& data = mImages[ url ];
		if( data.file.isNull()) return QByteArray();
		data.age = 0;
		return data.file;
	}
	return QByteArray();
}

void GVCache::getFrames( const KURL& url, GVImageFrames& frames, QCString& format ) const {
	LOG(url.prettyURL());
	frames=GVImageFrames();
	format=QCString();
	if( mImages.contains( url )) {
		const ImageData& data = mImages[ url ];
		if( data.frames.isEmpty()) return;
		frames = data.frames;
		format = data.format;
		data.age = 0;
	}
}

QPixmap GVCache::thumbnail( const KURL& url, QSize& imagesize ) const {
	if( mImages.contains( url )) {
		const ImageData& data = mImages[ url ];
		if( data.thumbnail.isNull()) return QPixmap();
		imagesize = data.imagesize;
//		data.age = 0;
		return data.thumbnail;
	}
	return QPixmap();
}

void GVCache::updateAge() {
	for( QMap< KURL, ImageData >::Iterator it = mImages.begin();
		it != mImages.end();
		++it ) {
		(*it).age++;
	}
}

void GVCache::checkThumbnailSize( int size ) {
	if( size != mThumbnailSize ) {
		// simply remove all thumbnails, should happen rarely
		for( QMap< KURL, ImageData >::Iterator it = mImages.begin();
			it != mImages.end();
			) {
			if( !(*it).thumbnail.isNull()) {
				QMap< KURL, ImageData >::Iterator it2 = it;
				++it;
				mImages.remove( it2 );
			} else {
				++it;
			}
		}
		mThumbnailSize = size;
	}
}

#ifdef DEBUG_CACHE
static KURL _cache_url; // hack only for debugging for item to show also its key
#endif

void GVCache::checkMaxSize() {
	for(;;) {
		int size = 0;
		QMap< KURL, ImageData >::Iterator max;
		long long max_cost = -1;
#ifdef DEBUG_CACHE
		int with_file = 0;
		int with_thumb = 0;
		int with_image = 0;
#endif
		for( QMap< KURL, ImageData >::Iterator it = mImages.begin();
			it != mImages.end();
			++it ) {
			size += (*it).size();
#ifdef DEBUG_CACHE
			if( !(*it).file.isNull()) ++with_file;
			if( !(*it).thumbnail.isNull()) ++with_thumb;
			if( !(*it).frames.isEmpty()) ++with_image;
#endif
			long long cost = (*it).cost();
			if( cost > max_cost ) {
				max_cost = cost;
				max = it;
			}
		}
		if( size <= mMaxSize ) {
#if 0
#ifdef DEBUG_CACHE
			kdDebug() << "GVCache: Statistics (" << mImages.size() << "/" << with_file << "/"
				<< with_thumb << "/" << with_image << ")" << endl;
#endif
#endif
			break;
		}
#ifdef DEBUG_CACHE
		_cache_url = max.key();
#endif
		if( !(*max).reduceSize() || (*max).isEmpty()) mImages.remove( max );
	}
}

void GVCache::readConfig(KConfig* config,const QString& group) {
	KConfigGroupSaver saver( config, group );
	mMaxSize = config->readNumEntry( CONFIG_CACHE_MAXSIZE, mMaxSize );
	checkMaxSize();
}

GVCache::ImageData::ImageData( const KURL& url, const QByteArray& f, const QDateTime& t )
: file( f )
, timestamp( t )
, age( 0 )
, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
{
	file.detach(); // explicit sharing
}

GVCache::ImageData::ImageData( const KURL& url, const GVImageFrames& frms, const QCString& f, const QDateTime& t )
: frames( frms )
, format( f )
, timestamp( t )
, age( 0 )
, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
{
}

GVCache::ImageData::ImageData( const KURL& url, const QPixmap& thumb, QSize imgsize, const QDateTime& t )
: thumbnail( thumb )
, imagesize( imgsize )
, timestamp( t )
, age( 0 )
, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
{
}

void GVCache::ImageData::addFile( const QByteArray& f ) {
	file = f;
	file.detach(); // explicit sharing
	age = 0;
}

void GVCache::ImageData::addImage( const GVImageFrames& fs, const QCString& f ) {
	frames = fs;
	format = f;
	age = 0;
}

void GVCache::ImageData::addThumbnail( const QPixmap& thumb, QSize imgsize ) {
	thumbnail = thumb;
	imagesize = imgsize;
//	age = 0;
}

int GVCache::ImageData::size() const {
	return QMAX( fileSize() + imageSize() + thumbnailSize(), 100 ); // some minimal size per item
}

int GVCache::ImageData::fileSize() const {
	return !file.isNull() ? file.size() : 0;
}

int GVCache::ImageData::thumbnailSize() const {
	return !thumbnail.isNull() ? thumbnail.height() * thumbnail.width() * thumbnail.depth() / 8 : 0;
}

int GVCache::ImageData::imageSize() const {
	int ret = 0;
	for( GVImageFrames::ConstIterator it = frames.begin(); it != frames.end(); ++it ) {
		ret += (*it).image.height() * (*it).image.width() * (*it).image.depth() / 8;
	}
	return ret;
}

bool GVCache::ImageData::reduceSize() {
	if( !file.isNull() && fast_url && !frames.isEmpty()) {
		file = QByteArray();
#ifdef DEBUG_CACHE
		kdDebug() << "GVCache: Dumping fast file: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		return true;
	}
	if( !thumbnail.isNull()) {
#ifdef DEBUG_CACHE
		kdDebug() << "GVCache: Dumping thumbnail: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		thumbnail = QPixmap();
		return true;
	}
	if( !file.isNull() && !frames.isEmpty()) {
	// possibly slow file to fetch - dump the image data unless the image
	// is JPEG (which needs raw data anyway) or the raw data much larger than the image
		if( format == "JPEG" || fileSize() < imageSize() / 10 ) {
			frames.clear();
#ifdef DEBUG_CACHE
			kdDebug() << "GVCache: Dumping images: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		} else {
			file = QByteArray();
#ifdef DEBUG_CACHE
			kdDebug() << "GVCache: Dumping file: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		}
		return true;
	}
#ifdef DEBUG_CACHE
	kdDebug() << "Cache: Dumping completely: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
	return false; // reducing here would mean clearing everything
}

bool GVCache::ImageData::isEmpty() const {
	return file.isNull() && frames.isEmpty() && thumbnail.isNull();
}

long long GVCache::ImageData::cost() const {
	long long s = size();
	if( fast_url && !file.isNull()) {
		s *= ( format == "JPEG" ? 10 : 100 ); // heavy penalty for storing local files
	} else if( !thumbnail.isNull()) {
		s *= 10 * 10; // thumbnails are small, and try to get rid of them soon
	}
	static const int mod[] = { 50, 30, 20, 16, 12, 10 };
	if( age <= 5 ) {
		return s * 10 / mod[ age ];
	} else {
		return s * ( age - 5 );
	}
}
