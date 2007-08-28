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

#include "imageloader.h"

#include <assert.h>

// Qt
#include <qtimer.h>
#include <qwmatrix.h>

// KDE
#include <kapplication.h>
#include <kimageio.h>
#include <kmimetype.h>

// Local
#include "cache.h"
#include "miscconfig.h"
#include "imageutils/imageutils.h"
#include "imageutils/jpegcontent.h"

#include "imageloader.moc"
namespace Gwenview {

const unsigned int DECODE_CHUNK_SIZE=4096;

/** Interval between image updates, in milli seconds */
const int IMAGE_UPDATE_INTERVAL=100;

#undef ENABLE_LOG
#undef LOG
#undef LOG2

#define ENABLE_LOG 0

#if ENABLE_LOG >= 1
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

#if ENABLE_LOG >= 2
#define LOG2(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG2(x) ;
#endif

static QMap< KURL, ImageLoader* > sLoaders;

//---------------------------------------------------------------------
//
// CancellableBuffer
// This class acts like QBuffer, but will simulates a truncated file if the
// TSThread which was passed to its constructor has been asked for cancellation
//
//---------------------------------------------------------------------
class CancellableBuffer : public QBuffer {
public:
	CancellableBuffer(QByteArray buffer, TSThread* thread)
	: QBuffer(buffer), mThread(thread) {}

	bool atEnd() const {
		if (mThread->testCancel()) {
			LOG("cancel detected");
			return true;
		}
		return QBuffer::atEnd();
	}
	
	Q_LONG readBlock(char * data, Q_ULONG maxlen) {
		if (mThread->testCancel()) {
			LOG("cancel detected");
			return 0;
		}
		return QBuffer::readBlock(data, maxlen);
	}

	Q_LONG readLine(char * data, Q_ULONG maxlen) {
		if (mThread->testCancel()) {
			LOG("cancel detected");
			return 0;
		}
		return QBuffer::readLine(data, maxlen);
	}

	QByteArray readAll() {
		if (mThread->testCancel()) {
			LOG("cancel detected");
			return QByteArray();
		}
		return QBuffer::readAll();
	}

	int getch() {
		if (mThread->testCancel()) {
			LOG("cancel detected");
			setStatus(IO_ReadError);
			return -1;
		}
		return QBuffer::getch();
	}

private:
	TSThread* mThread;
};


//---------------------------------------------------------------------
//
// DecoderThread
//
//---------------------------------------------------------------------
void DecoderThread::run() {
	QMutexLocker locker(&mMutex);
	LOG("");
	
	// This block makes sure imageIO won't access the image after the signal
	// has been posted
	{
		QImageIO imageIO;
		
		CancellableBuffer buffer(mRawData, this);
		buffer.open(IO_ReadOnly);
		imageIO.setIODevice(&buffer);
		bool ok=imageIO.read();
		if (testCancel()) {
			LOG("cancelled");
			return;
		}
			
		if (!ok) {
			LOG("failed");
			postSignal( this, SIGNAL(failed()) );
			return;
		}
		
		LOG("succeeded");
		mImage=imageIO.image();
	}
	
	LOG("succeeded, emitting signal");
	postSignal( this, SIGNAL(succeeded()) );
}


void DecoderThread::setRawData(const QByteArray& data) {
	QMutexLocker locker(&mMutex);
	mRawData=data.copy();
}


QImage DecoderThread::popLoadedImage() {
	QMutexLocker locker(&mMutex);
	QImage img=mImage;
	mImage=QImage();
	return img;
}
	


//---------------------------------------------------------------------
//
// ImageLoaderPrivate
//
//---------------------------------------------------------------------
struct OwnerData {
	const QObject* owner;
	BusyLevel priority;
};

enum GetState { 
	GET_PENDING_STAT, // Stat has not been started
	GET_STATING,      // Stat has been started
	GET_PENDING_GET,  // Stat is done, get has not been started
	GET_GETTING,      // Get has been started
	GET_DONE,         // All data has been received
};


enum DecodeState {
	DECODE_WAITING,                   // No data to decode yet
	DECODE_PENDING_THREADED_DECODING, // Waiting for all data to start threaded decoding
	DECODE_THREADED_DECODING,         // Threaded decoder is running
	DECODE_INCREMENTAL_DECODING,      // Incremental decoder is running
	DECODE_INCREMENTAL_DECODING_DONE, // Incremental decoder is done
	DECODE_CACHED,                    // Image has been obtained from cache, but raw data was missing. Wait for get to finish.
	DECODE_DONE,                      // All done
};

class ImageLoaderPrivate {
public:
	ImageLoaderPrivate(ImageLoader* impl)
	: mDecodedSize(0)
	, mGetState(GET_PENDING_STAT)
	, mDecodeState(DECODE_WAITING)
	, mDecoder(impl)
	, mSuspended(false)
	, mNextFrameDelay(0)
	, mWasFrameData(false)
	, mOrientation(ImageUtils::NOT_AVAILABLE)
	, mURLKind(MimeTypeUtils::KIND_UNKNOWN)
	{}

	// How many of the raw data we have already decoded
	unsigned int mDecodedSize;

	GetState mGetState;
	DecodeState mDecodeState;

	KURL mURL;

	// The file timestamp
	QDateTime mTimestamp;

	// The raw data we get
	QByteArray mRawData;
	
	// The async decoder and it's waking timer	
	QImageDecoder mDecoder;
	QTimer mDecoderTimer;

	// The decoder thread
	DecoderThread mDecoderThread;

	// A rect of recently loaded pixels that the rest of the application has
	// not been notified about with the imageChanged() signal
	QRect mLoadChangedRect; 
	
	// The time since we last emitted the imageChanged() signal
	QTime mTimeSinceLastUpdate;
	
	// Whether the loading should be suspended
	bool mSuspended;
	
	// Delay used for next frame after it's finished decoding.
	int mNextFrameDelay;

	bool mWasFrameData;

	QImage mProcessedImage; // image frame currently being decoded

	QRegion mLoadedRegion; // loaded parts of mProcessedImage

	ImageFrames mFrames;

	QCString mImageFormat;

	ImageUtils::Orientation mOrientation;
	
	QString mMimeType;
	MimeTypeUtils::Kind mURLKind;

	QValueVector< OwnerData > mOwners; // loaders may be shared
	
	
	void determineImageFormat() {
		Q_ASSERT(mRawData.size()>0);
		QBuffer buffer(mRawData);
		buffer.open(IO_ReadOnly);
		mImageFormat = QImageIO::imageFormat(&buffer);
	}
};


//---------------------------------------------------------------------
//
// ImageLoader
//
//---------------------------------------------------------------------
ImageLoader::ImageLoader() {
	LOG("");
	d = new ImageLoaderPrivate(this);
	connect( BusyLevelManager::instance(), SIGNAL( busyLevelChanged(BusyLevel)),
		this, SLOT( slotBusyLevelChanged(BusyLevel)));
}


ImageLoader::~ImageLoader() {
	LOG("");
	if (d->mDecoderThread.running()) {
		d->mDecoderThread.cancel();
		d->mDecoderThread.wait();
	}
	delete d;
}


void ImageLoader::setURL( const KURL& url ) {
	assert( d->mURL.isEmpty());
	d->mURL = url;
}

void ImageLoader::startLoading() {
	d->mTimestamp = Cache::instance()->timestamp( d->mURL );
	slotBusyLevelChanged( BusyLevelManager::instance()->busyLevel());

	connect(&d->mDecoderTimer, SIGNAL(timeout()), this, SLOT(decodeChunk()) );

	connect(&d->mDecoderThread, SIGNAL(succeeded()),
		this, SLOT(slotDecoderThreadSucceeded()) );
	connect(&d->mDecoderThread, SIGNAL(failed()),
		this, SLOT(slotDecoderThreadFailed()) );

	checkPendingStat();
}

void ImageLoader::checkPendingStat() {
	if( d->mSuspended || d->mGetState != GET_PENDING_STAT ) return;

	KIO::Job* job=KIO::stat( d->mURL, false );
	job->setWindow(KApplication::kApplication()->mainWidget());
	connect(job, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotStatResult(KIO::Job*)) );
	d->mGetState = GET_STATING;
}

void ImageLoader::slotStatResult(KIO::Job* job) {
	LOG("error code: " << job->error());

	// Get modification time of the original file
	KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
	KIO::UDSEntry::ConstIterator it= entry.begin();
	QDateTime urlTimestamp;
	for (; it!=entry.end(); it++) {
		if ((*it).m_uds == KIO::UDS_MODIFICATION_TIME) {
			urlTimestamp.setTime_t( (*it).m_long );
			break;
		}
	}

	if( d->mTimestamp.isValid() && urlTimestamp == d->mTimestamp ) {
		// We have the image in cache
		LOG(d->mURL << ", We have the image in cache");
		d->mRawData = Cache::instance()->file( d->mURL );
		Cache::instance()->getFrames(d->mURL, &d->mFrames, &d->mImageFormat);

		if( !d->mFrames.isEmpty()) {
			LOG("The image in cache can be used");
			d->mProcessedImage = d->mFrames[0].image;
			emit sizeLoaded(d->mProcessedImage.width(), d->mProcessedImage.height());
			emit imageChanged(d->mProcessedImage.rect());
			
			if (d->mRawData.isNull() && d->mImageFormat=="JPEG") {
				// Raw data is needed for JPEG, wait for it to be downloaded
				LOG("Wait for raw data to be downloaded");
				d->mDecodeState = DECODE_CACHED;
			} else {
				// We don't care about raw data
				finish(true);
				return;
			}
		} else {
			// Image in cache is broken
			LOG("The image in cache cannot be used");
			if( !d->mRawData.isNull()) {
				LOG("Using cached raw data");
				// Raw data is ok, skip get step and decode it
				d->mGetState = GET_DONE;
				d->mTimeSinceLastUpdate.start();
				d->mDecoderTimer.start(0, false);
				return;
			}
		}
	}

	d->mTimestamp = urlTimestamp;
	d->mRawData.resize(0);
	d->mGetState = GET_PENDING_GET;
	checkPendingGet();
}

void ImageLoader::checkPendingGet() {
	if( d->mSuspended || d->mGetState != GET_PENDING_GET ) return;

	// Start loading the image
	KIO::Job* getJob=KIO::get( d->mURL, false, false);
	getJob->setWindow(KApplication::kApplication()->mainWidget());

	connect(getJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
		this, SLOT(slotDataReceived(KIO::Job*, const QByteArray&)) );
	
	connect(getJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotGetResult(KIO::Job*)) );

	d->mTimeSinceLastUpdate.start();
	d->mGetState = GET_GETTING;
}
	

void ImageLoader::slotGetResult(KIO::Job* job) {
	LOG("error code: " << job->error());
	if( job->error() != 0 ) {
		// failed
		finish( false );
		return;
	}

	d->mGetState = GET_DONE;
	
	// Store raw data in cache
	// Note: Cache will give high cost to non-JPEG raw data.
	Cache::instance()->addFile( d->mURL, d->mRawData, d->mTimestamp );


	switch (d->mDecodeState) {
	case DECODE_CACHED:
		// image was in cache, but not raw data
		finish( true );
		break;

	case DECODE_PENDING_THREADED_DECODING:
		// Start the decoder thread if needed
		startThread();
		break;

	default:
		// Finish decoding if needed
		if (!d->mDecoderTimer.isActive()) d->mDecoderTimer.start(0);
	}
}

// There is no way in KImageIO to get the mimeType from the image format.
// This function assumes KImageIO::types and KImageIO::mimeTypes return items
// in the same order (which they do, according to the source code).
static QString mimeTypeFromFormat(const char* format) {
	QStringList formats = KImageIO::types(KImageIO::Reading);
	QStringList mimeTypes = KImageIO::mimeTypes(KImageIO::Reading);
	int pos = formats.findIndex(QString::fromAscii(format));
	Q_ASSERT(pos != -1);
	return mimeTypes[pos];
}

void ImageLoader::slotDataReceived(KIO::Job* job, const QByteArray& chunk) {
	LOG2("size: " << chunk.size());
	if (chunk.size()<=0) return;

	int oldSize=d->mRawData.size();
	d->mRawData.resize(oldSize + chunk.size());
	memcpy(d->mRawData.data()+oldSize, chunk.data(), chunk.size() );

	if (oldSize==0) {
		// Try to determine the data type
		QBuffer buffer(d->mRawData);
		buffer.open(IO_ReadOnly);
		const char* format = QImageIO::imageFormat(&buffer);
		if (format) {
			// This is a raster image, get the mime type now
			d->mURLKind = MimeTypeUtils::KIND_RASTER_IMAGE;
			d->mMimeType = mimeTypeFromFormat(format);
		} else {
			KMimeType::Ptr ptr = KMimeType::findByContent(d->mRawData);
			d->mMimeType = ptr->name();
			d->mURLKind = MimeTypeUtils::mimeTypeKind(d->mMimeType);
		}
		if (d->mURLKind!=MimeTypeUtils::KIND_RASTER_IMAGE) {
			Q_ASSERT(!d->mDecoderTimer.isActive());
			job->kill(true /* quietly */);
			LOG("emit urlKindDetermined(!raster)");
			emit urlKindDetermined();
			return;
		}
		LOG("emit urlKindDetermined(raster)");
		emit urlKindDetermined();
	}
	
	// Decode the received data
	if( !d->mDecoderTimer.isActive() && 
		(d->mDecodeState==DECODE_WAITING || d->mDecodeState==DECODE_INCREMENTAL_DECODING)
	) {
		d->mDecoderTimer.start(0);
	}
}


void ImageLoader::decodeChunk() {
	if( d->mSuspended ) {
		LOG("suspended");
		d->mDecoderTimer.stop();
		return;
	}

	int chunkSize = QMIN(DECODE_CHUNK_SIZE, int(d->mRawData.size())-d->mDecodedSize);
	int decodedSize = 0;
	if (chunkSize>0) {
		decodedSize = d->mDecoder.decode(
			(const uchar*)(d->mRawData.data()+d->mDecodedSize),
			chunkSize);

		if (decodedSize<0) {
			// We can't use incremental decoding, switch to threaded decoding
			d->mDecoderTimer.stop();
			if (d->mGetState == GET_DONE) {
				startThread();
			} else {
				d->mDecodeState = DECODE_PENDING_THREADED_DECODING;
			}
			return;
		}

		// We just decoded some data
		if (d->mDecodeState == DECODE_WAITING) {
			d->mDecodeState = DECODE_INCREMENTAL_DECODING;
		}
		d->mDecodedSize+=decodedSize;
	}

	if (decodedSize == 0) {
		// We decoded as much as possible from the buffer, wait to receive
		// more data before coming again in decodeChunk
		d->mDecoderTimer.stop();

		if (d->mGetState == GET_DONE) {
			// All available data has been received.
			if (d->mDecodeState == DECODE_INCREMENTAL_DECODING) {
				// Decoder is not finished, the image must be truncated,
				// let's simulate its end
				kdWarning() << "ImageLoader::decodeChunk(): image '" << d->mURL.prettyURL() << "' is truncated.\n";

				if (d->mProcessedImage.isNull()) {
					d->mProcessedImage = d->mDecoder.image();
				}
				emit imageChanged(d->mProcessedImage.rect());
				end();
			}
		}
	}
}


void ImageLoader::startThread() {
	LOG("starting decoder thread");
	d->mDecodeState = DECODE_THREADED_DECODING;
	d->mDecoderThread.setRawData(d->mRawData);
	d->mDecoderThread.start();
}


void ImageLoader::slotDecoderThreadFailed() {
	LOG("");
	// Image can't be loaded
	finish( false );
}


void ImageLoader::slotDecoderThreadSucceeded() {
	LOG("");
	d->mProcessedImage = d->mDecoderThread.popLoadedImage();
	d->mFrames.append( ImageFrame( d->mProcessedImage, 0 ));
	emit sizeLoaded(d->mProcessedImage.width(), d->mProcessedImage.height());
	emit imageChanged(d->mProcessedImage.rect());
	finish(true);
}


/**
 * Cache image and emit imageLoaded
 */
void ImageLoader::finish( bool ok ) {
	LOG("");

	d->mDecodeState = DECODE_DONE;

	if (!ok) {
		d->mFrames.clear();
		d->mRawData = QByteArray();
		d->mImageFormat = QCString();
		d->mProcessedImage = QImage();
		emit imageLoaded( false );
		return;
	}
	
	if (d->mImageFormat.isEmpty()) {
		d->determineImageFormat();
	}
	Q_ASSERT(d->mFrames.count() > 0);
	Cache::instance()->addImage( d->mURL, d->mFrames, d->mImageFormat, d->mTimestamp );
	emit imageLoaded( true );
}


BusyLevel ImageLoader::priority() const {
	BusyLevel mylevel = BUSY_NONE;
	for( QValueVector< OwnerData >::ConstIterator it = d->mOwners.begin();
			it != d->mOwners.end();
			++it ) {
		mylevel = QMAX( mylevel, (*it).priority );
	}
	return mylevel;
}

void ImageLoader::slotBusyLevelChanged( BusyLevel level ) {
	// this loader may be needed for normal loading (BUSY_LOADING), or
	// only for prefetching
	BusyLevel mylevel = priority();
	if( level > mylevel ) {
		suspendLoading();
	} else {
		resumeLoading();
	}
}

void ImageLoader::suspendLoading() {
	d->mDecoderTimer.stop();
	d->mSuspended = true;
}

void ImageLoader::resumeLoading() {
	d->mSuspended = false;
	d->mDecoderTimer.start(0, false);
	checkPendingGet();
	checkPendingStat();
}


//---------------------------------------------------------------------
//
// QImageConsumer
//
//---------------------------------------------------------------------
void ImageLoader::end() {
	LOG("");

	// Notify about the last loaded rectangle
	LOG("mLoadChangedRect " << d->mLoadChangedRect);
	if (!d->mLoadChangedRect.isEmpty()) {
		emit imageChanged( d->mLoadChangedRect );
	}

	d->mDecoderTimer.stop();
	d->mDecodeState = DECODE_INCREMENTAL_DECODING_DONE;

	// We are done
	if( d->mFrames.count() == 0 ) {
		d->mFrames.append( ImageFrame( d->mProcessedImage, 0 ));
	}
	// The image has been totally decoded, we delay the call to finish because
	// when we return from this function we will be in decodeChunk(), after the
	// call to decode(), so we don't want to switch to a new impl yet, since
	// this means deleting "this".
	QTimer::singleShot(0, this, SLOT(callFinish()) );
}


void ImageLoader::callFinish() {
	finish(true);
}


void ImageLoader::changed(const QRect& constRect) {
	LOG2("");
	QRect rect = constRect;
	
	if (d->mLoadedRegion.isEmpty()) {
		// This is the first time we get called. Init mProcessedImage and emit
		// sizeLoaded.
		LOG("mLoadedRegion is empty");
		
		// By default, mProcessedImage should use the image from mDecoder
		d->mProcessedImage = d->mDecoder.image();
		
		if (d->mImageFormat.isEmpty()) {
			d->determineImageFormat();
		}
		Q_ASSERT(!d->mImageFormat.isEmpty());
		if (d->mImageFormat == "JPEG") {
			// This is a JPEG, extract orientation and adjust mProcessedImage
			// if necessary according to misc options
			ImageUtils::JPEGContent content;

			if (content.loadFromData(d->mRawData)) {
				d->mOrientation = content.orientation(); 
				if (MiscConfig::autoRotateImages() && 
					d->mOrientation != ImageUtils::NOT_AVAILABLE && d->mOrientation != ImageUtils::NORMAL) {
					QSize size = content.size();
					d->mProcessedImage = QImage(size, d->mDecoder.image().depth());
				}
				d->mProcessedImage.setDotsPerMeterX(content.dotsPerMeterX());
				d->mProcessedImage.setDotsPerMeterY(content.dotsPerMeterY());
			} else {
				kdWarning() << "ImageLoader::changed(): JPEGContent could not load '" << d->mURL.prettyURL() << "'\n";
			}
		}
		
		LOG("emit sizeLoaded " << d->mProcessedImage.size());
		emit sizeLoaded(d->mProcessedImage.width(), d->mProcessedImage.height());
	}

	// Apply orientation if necessary and if wanted by user settings (misc options)
	if (MiscConfig::autoRotateImages() && 
		d->mOrientation != ImageUtils::NOT_AVAILABLE && d->mOrientation != ImageUtils::NORMAL) {
		// We can only rotate whole images, so copy the loaded rect in a temp
		// image, rotate the temp image and copy it to mProcessedImage

		// Copy loaded rect
		QImage temp(rect.size(), d->mProcessedImage.depth());
		bitBlt(&temp, 0, 0, 
			&d->mDecoder.image(), rect.left(), rect.top(), rect.width(), rect.height());
		
		// Rotate
		temp = ImageUtils::transform(temp, d->mOrientation);

		// Compute destination rect
		QWMatrix matrix = ImageUtils::transformMatrix(d->mOrientation);
		
		QRect imageRect = d->mDecoder.image().rect();
		imageRect = matrix.mapRect(imageRect);
				
		rect = matrix.mapRect(rect);
		rect.moveBy(-imageRect.left(), -imageRect.top());
		
		// copy temp to mProcessedImage
		bitBlt(&d->mProcessedImage, rect.left(), rect.top(),
			&temp, 0, 0, temp.width(), temp.height());
	}
	
	// Update state tracking vars
	d->mWasFrameData = true;
	d->mLoadChangedRect |= rect;
	d->mLoadedRegion |= rect;
	if( d->mTimeSinceLastUpdate.elapsed() > IMAGE_UPDATE_INTERVAL ) {
		LOG("emitting imageChanged " << d->mLoadChangedRect);
		d->mTimeSinceLastUpdate.start();
		emit imageChanged(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
	}
}

void ImageLoader::frameDone() {
	frameDone( QPoint( 0, 0 ), d->mDecoder.image().rect());
}

void ImageLoader::frameDone(const QPoint& offset, const QRect& rect) {
	LOG("");
	// Another case where the image loading in Qt's is a bit borken.
	// It's possible to get several notes about a frame being done for one frame (with MNG).
	if( !d->mWasFrameData ) {
		// To make it even more fun, with MNG the sequence is actually
		// setFramePeriod( 0 )
		// frameDone()
		// setFramePeriod( delay )
		// frameDone()
		// Therefore ignore the second frameDone(), but fix the delay that should be
		// after the frame.
		if( d->mFrames.count() > 0 ) {
			d->mFrames.last().delay = d->mNextFrameDelay;
			d->mNextFrameDelay = 0;
		}
		return;
	}
	d->mWasFrameData = false;
	if( !d->mLoadChangedRect.isEmpty()) {
		emit imageChanged(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
		d->mTimeSinceLastUpdate.start();
	}
	d->mLoadedRegion = QRegion();
	
	QImage image;
	if (d->mProcessedImage.isNull()) {
		image = d->mDecoder.image().copy();
	} else {
		image = d->mProcessedImage.copy();
	}
	
	if( offset != QPoint( 0, 0 ) || rect != image.rect()) {
		// Blit last frame below 'image'
		if( !d->mFrames.isEmpty()) {
			QImage im = d->mFrames.last().image.copy();
			bitBlt( &im, offset.x(), offset.y(), &image, rect.x(), rect.y(), rect.width(), rect.height());
			image = im;
		}
	}
	d->mFrames.append( ImageFrame( image, d->mNextFrameDelay ));
	d->mNextFrameDelay = 0;
}

void ImageLoader::setLooping(int) {
}

void ImageLoader::setFramePeriod(int milliseconds) {
	if( milliseconds < 0 ) milliseconds = 0; // -1 means showing immediately
	if( d->mNextFrameDelay == 0 || milliseconds != 0 ) {
		d->mNextFrameDelay = milliseconds;
	}
}

void ImageLoader::setSize(int, int) {
	// Do nothing, size is handled when ::changed() is called for the first
	// time
}


QImage ImageLoader::processedImage() const {
	return d->mProcessedImage;
}


ImageFrames ImageLoader::frames() const {
	return d->mFrames;
}


QCString ImageLoader::imageFormat() const {
	return d->mImageFormat;
}


QByteArray ImageLoader::rawData() const {
	return d->mRawData;
}


QString ImageLoader::mimeType() const {
	return d->mMimeType;
}


MimeTypeUtils::Kind ImageLoader::urlKind() const {
	return d->mURLKind;
}


KURL ImageLoader::url() const {
	return d->mURL;
}


QRegion ImageLoader::loadedRegion() const {
	return d->mLoadedRegion;
}


bool ImageLoader::completed() const {
	return d->mDecodeState == DECODE_DONE;
}


void ImageLoader::ref( const QObject* owner, BusyLevel priority ) {
	OwnerData data;
	data.owner = owner;
	data.priority = priority;
	d->mOwners.append( data );
	connect( owner, SIGNAL( destroyed()), SLOT( ownerDestroyed()));
}

void ImageLoader::deref( const QObject* owner ) {
	for( QValueVector< OwnerData >::Iterator it = d->mOwners.begin();
	     it != d->mOwners.end();
	     ++it ) {
		if( (*it).owner == owner ) {
			d->mOwners.erase( it );
			if( d->mOwners.count() == 0 ) {
				sLoaders.remove( d->mURL );
				delete this;
			}
			return;
		}
	}
	assert( false );
}

void ImageLoader::release( const QObject* owner ) {
	disconnect( owner );
	deref( owner );
}

void ImageLoader::ownerDestroyed() {
	deref( sender());
}

//---------------------------------------------------------------------
//
// Managing loaders
//
//---------------------------------------------------------------------

ImageLoader* ImageLoader::loader( const KURL& url, const QObject* owner, BusyLevel priority ) {
	if( sLoaders.contains( url )) {
		ImageLoader* loader = sLoaders[ url ];
		loader->ref( owner, priority );
		// resume if this owner has high priority
		loader->slotBusyLevelChanged( BusyLevelManager::instance()->busyLevel());
		return loader;
	}
	ImageLoader* loader = new ImageLoader;
	loader->ref( owner, priority );
	sLoaders[ url ] = loader;
	loader->setURL( url );
	// Code using a loader first calls loader() to get ImageLoader* and only after that it can
	// connect to its signals etc., so don't start loading immediately.
	// This also helps with preloading jobs, since BUSY_LOADING busy level is not entered immediately
	// when a new picture is selected, so preloading jobs without this delay could start working
	// immediately.
	QTimer::singleShot( priority >= BUSY_LOADING ? 0 : 10, loader, SLOT( startLoading()));
	return loader;
}

} // namespace
