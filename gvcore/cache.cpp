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
#include <ksharedptr.h>
#include <kstaticdeleter.h>
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


struct ImageData : public KShared {
	ImageData( const KURL& url, const QDateTime& _timestamp )
	: timestamp(_timestamp)
	, age(0)
	, fast_url( url.isLocalFile() && !KIO::probably_slow_mounted( url.path()))
	, priority( false ) {
	}

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

	typedef KSharedPtr<ImageData> Ptr;
};

typedef QMap<KURL, ImageData::Ptr> ImageMap;

struct Cache::Private {
	ImageMap mImages;
	int mMaxSize;
	int mThumbnailSize;
	QValueList< KURL > mPriorityURLs;


	/**
	 * This function tries to returns a valid ImageData for url and timestamp.
	 * If it can't find one, it will create a new one and return it.
	 */
	ImageData::Ptr getOrCreateImageData(const KURL& url, const QDateTime& timestamp) {
		if (mImages.contains(url)) {
			ImageData::Ptr data = mImages[url];
			if (data->timestamp == timestamp) return data;
		}

		ImageData::Ptr data = new ImageData(url, timestamp);
		mImages[url] = data;
		if (mPriorityURLs.contains(url)) data->priority = true;
		return data;
	}
};


Cache::Cache()
{
	d = new Private;
	d->mMaxSize = DEFAULT_MAXSIZE;
	// don't remember size for every thumbnail, but have one global and dump all if needed
	d->mThumbnailSize = 0;
}


Cache::~Cache() {
	d->mImages.clear();
	delete d;
}


static Cache* sCache;
static KStaticDeleter<Cache> sCacheDeleter;


Cache* Cache::instance() {
	if (!sCache) {
		sCacheDeleter.setObject(sCache, new Cache());
	}
	return sCache;
}

// Priority URLs are used e.g. when prefetching for the slideshow - after an image is prefetched,
// the loader tries to put the image in the cache. When the slideshow advances, the next loader
// just gets the image from the cache. However, the prefetching may be useless if the image
// actually doesn't stay long enough in the cache, e.g. because of being too big for the cache.
// Marking an URL as a priority one will make sure it stays in the cache and that the cache
// will be even enlarged as necessary if needed.
void Cache::setPriorityURL( const KURL& url, bool set ) {
	if( set ) {
		d->mPriorityURLs.append( url );
		if( d->mImages.contains( url )) {
			d->mImages[ url ]->priority = true;
		}
	} else {
		d->mPriorityURLs.remove( url );
		if( d->mImages.contains( url )) {
			d->mImages[ url ]->priority = false;
		}
		checkMaxSize();
	}
}


void Cache::addFile( const KURL& url, const QByteArray& file, const QDateTime& timestamp ) {
	LOG(url.prettyURL());
	updateAge();
	d->getOrCreateImageData(url, timestamp)->addFile(file);
	checkMaxSize();
}

void Cache::addImage( const KURL& url, const ImageFrames& frames, const QCString& format, const QDateTime& timestamp ) {
	LOG(url.prettyURL());
	updateAge();
	d->getOrCreateImageData(url, timestamp)->addImage(frames, format);
	checkMaxSize();
}

void Cache::addThumbnail( const KURL& url, const QPixmap& thumbnail, QSize imagesize, const QDateTime& timestamp ) {
// Thumbnails are many and often - things would age too quickly. Therefore
// when adding thumbnails updateAge() is called from the outside only once for all of them.
//	updateAge();
	d->getOrCreateImageData(url, timestamp)->addThumbnail(thumbnail, imagesize);
	checkMaxSize();
}

void Cache::invalidate( const KURL& url ) {
	d->mImages.remove( url );
}

QDateTime Cache::timestamp( const KURL& url ) const {
	LOG(url.prettyURL());
	if( d->mImages.contains( url )) return d->mImages[ url ]->timestamp;
	return QDateTime();
}

QByteArray Cache::file( const KURL& url ) const {
	LOG(url.prettyURL());
	if( d->mImages.contains( url )) {
		const ImageData::Ptr data = d->mImages[ url ];
		if( data->file.isNull()) return QByteArray();
		data->age = 0;
		return data->file;
	}
	return QByteArray();
}

void Cache::getFrames( const KURL& url, ImageFrames* frames, QCString* format ) const {
	LOG(url.prettyURL());
	Q_ASSERT(frames);
	Q_ASSERT(format);
	frames->clear();
	*format = QCString();
	if( d->mImages.contains( url )) {
		const ImageData::Ptr data = d->mImages[ url ];
		if( data->frames.isEmpty()) return;
		*frames = data->frames;
		*format = data->format;
		data->age = 0;
	}
}

QPixmap Cache::thumbnail( const KURL& url, QSize& imagesize ) const {
	if( d->mImages.contains( url )) {
		const ImageData::Ptr data = d->mImages[ url ];
		if( data->thumbnail.isNull()) return QPixmap();
		imagesize = data->imagesize;
//		data.age = 0;
		return data->thumbnail;
	}
	return QPixmap();
}

void Cache::updateAge() {
	for( ImageMap::Iterator it = d->mImages.begin();
		it != d->mImages.end();
		++it ) {
		(*it)->age++;
	}
}

void Cache::checkThumbnailSize( int size ) {
	if( size != d->mThumbnailSize ) {
		// simply remove all thumbnails, should happen rarely
		for( ImageMap::Iterator it = d->mImages.begin();
			it != d->mImages.end();
			) {
			if( !(*it)->thumbnail.isNull()) {
				ImageMap::Iterator it2 = it;
				++it;
				d->mImages.remove( it2 );
			} else {
				++it;
			}
		}
		d->mThumbnailSize = size;
	}
}

#ifdef DEBUG_CACHE
static KURL _cache_url; // hack only for debugging for item to show also its key
#endif

void Cache::checkMaxSize() {
	for(;;) {
		int size = 0;
		ImageMap::Iterator max;
		long long max_cost = -1;
#ifdef DEBUG_CACHE
		int with_file = 0;
		int with_thumb = 0;
		int with_image = 0;
#endif
		for( ImageMap::Iterator it = d->mImages.begin();
			it != d->mImages.end();
			++it ) {
			size += (*it)->size();
#ifdef DEBUG_CACHE
			if( !(*it).file.isNull()) ++with_file;
			if( !(*it).thumbnail.isNull()) ++with_thumb;
			if( !(*it).frames.isEmpty()) ++with_image;
#endif
			long long cost = (*it)->cost();
			if( cost > max_cost && ! (*it)->priority ) {
				max_cost = cost;
				max = it;
			}
		}
		if( size <= d->mMaxSize || max_cost == -1 ) {
#if 0
#ifdef DEBUG_CACHE
			kdDebug() << "Cache: Statistics (" << d->mImages.size() << "/" << with_file << "/"
				<< with_thumb << "/" << with_image << ")" << endl;
#endif
#endif
			break;
		}
#ifdef DEBUG_CACHE
		_cache_url = max.key();
#endif

		if( !(*max)->reduceSize() || (*max)->isEmpty()) d->mImages.remove( max );
	}
}

void Cache::readConfig(KConfig* config,const QString& group) {
	KConfigGroupSaver saver( config, group );
	d->mMaxSize = config->readNumEntry( CONFIG_CACHE_MAXSIZE, d->mMaxSize );
	checkMaxSize();
}


void ImageData::addFile( const QByteArray& f ) {
	file = f;
	file.detach(); // explicit sharing
	age = 0;
}

void ImageData::addImage( const ImageFrames& fs, const QCString& f ) {
	frames = fs;
	format = f;
	age = 0;
}

void ImageData::addThumbnail( const QPixmap& thumb, QSize imgsize ) {
	thumbnail = thumb;
	imagesize = imgsize;
//	age = 0;
}

int ImageData::size() const {
	return QMAX( fileSize() + imageSize() + thumbnailSize(), 100 ); // some minimal size per item
}

int ImageData::fileSize() const {
	return !file.isNull() ? file.size() : 0;
}

int ImageData::thumbnailSize() const {
	return !thumbnail.isNull() ? thumbnail.height() * thumbnail.width() * thumbnail.depth() / 8 : 0;
}

int ImageData::imageSize() const {
	int ret = 0;
	for( ImageFrames::ConstIterator it = frames.begin(); it != frames.end(); ++it ) {
		ret += (*it).image.height() * (*it).image.width() * (*it).image.depth() / 8;
	}
	return ret;
}

bool ImageData::reduceSize() {
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

bool ImageData::isEmpty() const {
	return file.isNull() && frames.isEmpty() && thumbnail.isNull();
}

long long ImageData::cost() const {
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
