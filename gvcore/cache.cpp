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

#include "cache.h"

// Qt

// KDE
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kio/global.h>

#include "cache.moc"

namespace Gwenview {

// Local

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

//#define DEBUG_CACHE

const char CONFIG_CACHE_MAXSIZE[]="maxSize";

Cache::Cache()
: mMaxSize( DEFAULT_MAXSIZE )
, mThumbnailSize( 0 ) // don't remember size for every thumbnail, but have one global and dump all if needed
, mUsageRefcount( 0 )
{
    connect( &mCleanupTimer, SIGNAL( timeout()), SLOT( cleanupTimeout()));
}

Cache* Cache::instance() {
	static Cache manager;
	return &manager;
}

// Priority URLs are used e.g. when prefetching for the slideshow - after an image is prefetched,
// the loader tries to put the image in the cache. When the slideshow advances, the next loader
// just gets the image from the cache. However, the prefetching may be useless if the image
// actually doesn't stay long enough in the cache, e.g. because of being too big for the cache.
// Marking an URL as a priority one will make sure it stays in the cache and that the cache
// will be even enlarged as necessary if needed.
void Cache::setPriorityURL( const KURL& url, bool set ) {
	if( set ) {
		mPriorityURLs.append( url );
		if( mImages.contains( url )) {
			mImages[ url ].priority = true;
		}
	} else {
		mPriorityURLs.remove( url );
		if( mImages.contains( url )) {
			mImages[ url ].priority = false;
		}
		checkMaxSize();
	}
}

void Cache::addFile( const KURL& url, const QByteArray& file, const QDateTime& timestamp ) {
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
	if( insert ) {
		mImages[ url ] = ImageData( url, file, timestamp );
		if( mPriorityURLs.contains( url )) mImages[ url ].priority = true;
	}
	checkMaxSize();
}

void Cache::addImage( const KURL& url, const ImageFrames& frames, const QCString& format, const QDateTime& timestamp ) {
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
	if( insert ) {
		mImages[ url ] = ImageData( url, frames, format, timestamp );
		if( mPriorityURLs.contains( url )) mImages[ url ].priority = true;
	}
	checkMaxSize();
}

void Cache::addThumbnail( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp ) {
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
	if( insert ) {
		mImages[ url ] = ImageData( url, thumbnail, imagesize, timestamp );
		if( mPriorityURLs.contains( url )) mImages[ url ].priority = true;
	}
	checkMaxSize();
}

QDateTime Cache::timestamp( const KURL& url ) const {
	LOG(url.prettyURL());
	if( mImages.contains( url )) return mImages[ url ].timestamp;
	return QDateTime();
}

QByteArray Cache::file( const KURL& url ) const {
	LOG(url.prettyURL());
	if( mImages.contains( url )) {
		const ImageData& data = mImages[ url ];
		if( data.file.isNull()) return QByteArray();
		data.age = 0;
		return data.file;
	}
	return QByteArray();
}

void Cache::getFrames( const KURL& url, ImageFrames& frames, QCString& format ) const {
	LOG(url.prettyURL());
	frames=ImageFrames();
	format=QCString();
	if( mImages.contains( url )) {
		const ImageData& data = mImages[ url ];
		if( data.frames.isEmpty()) return;
		frames = data.frames;
		format = data.format;
		data.age = 0;
	}
}

QPixmap Cache::thumbnail( const KURL& url, QSize& imagesize ) const {
	if( mImages.contains( url )) {
		const ImageData& data = mImages[ url ];
		if( data.thumbnail.isNull()) return QPixmap();
		imagesize = data.imagesize;
//		data.age = 0;
		return data.thumbnail;
	}
	return QPixmap();
}

void Cache::updateAge() {
	for( QMap< KURL, ImageData >::Iterator it = mImages.begin();
		it != mImages.end();
		++it ) {
		(*it).age++;
	}
}

void Cache::checkThumbnailSize( int size ) {
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

void Cache::checkMaxSize() {
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
			if( cost > max_cost && ! (*it).priority ) {
				max_cost = cost;
				max = it;
			}
		}
		if( size <= mMaxSize || max_cost == -1 ) {
#if 0
#ifdef DEBUG_CACHE
			kdDebug() << "Cache: Statistics (" << mImages.size() << "/" << with_file << "/"
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

void Cache::readConfig(KConfig* config,const QString& group) {
	KConfigGroupSaver saver( config, group );
	mMaxSize = config->readNumEntry( CONFIG_CACHE_MAXSIZE, mMaxSize );
	checkMaxSize();
}

void Cache::ref() {
	++mUsageRefcount;
	mCleanupTimer.stop();
}

void Cache::deref() {
	if( --mUsageRefcount == 0 ) mCleanupTimer.start( 60000, true );
}

void Cache::cleanupTimeout() {
	mImages.clear();
}

Cache::ImageData::ImageData( const KURL& url, const QByteArray& f, const QDateTime& t )
: file( f )
, timestamp( t )
, age( 0 )
, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
, priority( false )
{
	file.detach(); // explicit sharing
}

Cache::ImageData::ImageData( const KURL& url, const ImageFrames& frms, const QCString& f, const QDateTime& t )
: frames( frms )
, format( f )
, timestamp( t )
, age( 0 )
, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
, priority( false )
{
}

Cache::ImageData::ImageData( const KURL& url, const QPixmap& thumb, QSize imgsize, const QDateTime& t )
: thumbnail( thumb )
, imagesize( imgsize )
, timestamp( t )
, age( 0 )
, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
, priority( false )
{
}

void Cache::ImageData::addFile( const QByteArray& f ) {
	file = f;
	file.detach(); // explicit sharing
	age = 0;
}

void Cache::ImageData::addImage( const ImageFrames& fs, const QCString& f ) {
	frames = fs;
	format = f;
	age = 0;
}

void Cache::ImageData::addThumbnail( const QPixmap& thumb, QSize imgsize ) {
	thumbnail = thumb;
	imagesize = imgsize;
//	age = 0;
}

int Cache::ImageData::size() const {
	return QMAX( fileSize() + imageSize() + thumbnailSize(), 100 ); // some minimal size per item
}

int Cache::ImageData::fileSize() const {
	return !file.isNull() ? file.size() : 0;
}

int Cache::ImageData::thumbnailSize() const {
	return !thumbnail.isNull() ? thumbnail.height() * thumbnail.width() * thumbnail.depth() / 8 : 0;
}

int Cache::ImageData::imageSize() const {
	int ret = 0;
	for( ImageFrames::ConstIterator it = frames.begin(); it != frames.end(); ++it ) {
		ret += (*it).image.height() * (*it).image.width() * (*it).image.depth() / 8;
	}
	return ret;
}

bool Cache::ImageData::reduceSize() {
	if( !file.isNull() && fast_url && !frames.isEmpty()) {
		file = QByteArray();
#ifdef DEBUG_CACHE
		kdDebug() << "Cache: Dumping fast file: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		return true;
	}
	if( !thumbnail.isNull()) {
#ifdef DEBUG_CACHE
		kdDebug() << "Cache: Dumping thumbnail: " << _cache_url.prettyURL() << ":" << cost() << endl;
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
			kdDebug() << "Cache: Dumping images: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		} else {
			file = QByteArray();
#ifdef DEBUG_CACHE
			kdDebug() << "Cache: Dumping file: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
		}
		return true;
	}
#ifdef DEBUG_CACHE
	kdDebug() << "Cache: Dumping completely: " << _cache_url.prettyURL() << ":" << cost() << endl;
#endif
	return false; // reducing here would mean clearing everything
}

bool Cache::ImageData::isEmpty() const {
	return file.isNull() && frames.isEmpty() && thumbnail.isNull();
}

long long Cache::ImageData::cost() const {
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

} // namespace
