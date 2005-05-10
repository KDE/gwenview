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

#include "gvimageloader.h"

// Qt
#include <qtimer.h>

// KDE

// Local
#include "gvcache.h"

#include "gvimageloader.moc"

const unsigned int DECODE_CHUNK_SIZE=4096;

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

static QMap< KURL, GVImageLoader* > loaders;

//---------------------------------------------------------------------
//
// GVCancellableBuffer
// This class acts like QBuffer, but will simulates a truncated file if the
// TSThread which was passed to its constructor has been asked for cancellation
//
//---------------------------------------------------------------------
class GVCancellableBuffer : public QBuffer {
public:
	GVCancellableBuffer(QByteArray buffer, TSThread* thread)
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
// GVDecoderThread
//
//---------------------------------------------------------------------
void GVDecoderThread::run() {
	QMutexLocker locker(&mMutex);
	LOG("");
	
	// This block makes sure imageIO won't access the image after the signal
	// has been posted
	{
		QImageIO imageIO;
		
		GVCancellableBuffer buffer(mRawData, this);
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


void GVDecoderThread::setRawData(const QByteArray& data) {
	QMutexLocker locker(&mMutex);
	mRawData=data.copy();
}


QImage GVDecoderThread::popLoadedImage() {
	QMutexLocker locker(&mMutex);
	QImage img=mImage;
	mImage=QImage();
	return img;
}
	


//---------------------------------------------------------------------
//
// GVImageLoaderPrivate
//
//---------------------------------------------------------------------
class GVImageLoaderPrivate {
public:
	GVImageLoaderPrivate(GVImageLoader* impl)
	: mDecodedSize(0)
	, mUseThread(false)
	, mDecoder(impl)
	, mUpdatedDuringLoad(false)
	, mSuspended(false)
	, mGetComplete(false)
	, mAsyncImageComplete(false)
	, mNextFrameDelay(0)
	, mWasFrameData(false)
	, mRefcount( 0 )
	, mDecodeComplete( false )
	{}

	KURL mURL;

	// The file timestamp
	QDateTime mTimestamp;

	// The raw data we get
	QByteArray mRawData;

	// How many of the raw data we have already decoded
	unsigned int mDecodedSize;
	
	// Whether we are using a thread to decode the image (if the async decoder
	// failed)
	bool mUseThread;

	// The async decoder and it's waking timer	
	QImageDecoder mDecoder;
	QTimer mDecoderTimer;

	// The decoder thread
	GVDecoderThread mDecoderThread;
	
	// Set to true if at least one changed() signals have been emitted
	bool mUpdatedDuringLoad;
	// Set to image size if sizeLoaded() signal has been emitted
	QSize mKnownSize;

	// A rect of recently loaded pixels that the rest of the application has
	// not been notified about with the imageChanged() signal
	QRect mLoadChangedRect; 
	
	// The time since we last emitted the imageChanged() signal
	QTime mTimeSinceLastUpdate;
	
	// Whether the loading should be suspended
	bool mSuspended;
	
	// Set to true when all the raw data has been received
	bool mGetComplete;
	
	// Set to true when all the image has been decoded
	bool mAsyncImageComplete;

	// Delay used for next frame after it's finished decoding.
	int mNextFrameDelay;

	bool mWasFrameData;

	QImage mProcessedImage; // image frame currently being decoded

	QRegion mLoadedRegion; // loaded parts of mProcessedImage

	GVImageFrames mFrames;

	QCString mImageFormat;

	int mRefcount; // loaders may be shared

	bool mDecodeComplete; // is decoding fully completed?
};


//---------------------------------------------------------------------
//
// GVImageLoader
//
//---------------------------------------------------------------------
GVImageLoader::GVImageLoader() {
	LOG("");
	d = new GVImageLoaderPrivate(this);
	connect( GVBusyLevelManager::instance(), SIGNAL( busyLevelChanged(GVBusyLevel)),
		this, SLOT( slotBusyLevelChanged(GVBusyLevel)));
}


GVImageLoader::~GVImageLoader() {
	LOG("");
	if (d->mDecoderThread.running()) {
		d->mDecoderThread.cancel();
		d->mDecoderThread.wait();
	}
	delete d;
}


void GVImageLoader::startLoading( const KURL& url ) {
	if( !d->mURL.isEmpty()) return; // already loading
	d->mURL = url;
	d->mTimestamp = GVCache::instance()->timestamp( d->mURL );
	slotBusyLevelChanged( GVBusyLevelManager::instance()->busyLevel());

	connect(&d->mDecoderTimer, SIGNAL(timeout()), this, SLOT(decodeChunk()) );

	connect(&d->mDecoderThread, SIGNAL(succeeded()),
		this, SLOT(slotImageDecoded()) );
	connect(&d->mDecoderThread, SIGNAL(failed()),
		this, SLOT(slotDecoderThreadFailed()) );

	KIO::Job* job=KIO::stat( d->mURL, false );
	connect(job, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotStatResult(KIO::Job*)) );
}

void GVImageLoader::slotStatResult(KIO::Job* job) {
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
		QCString format;
		d->mRawData = GVCache::instance()->file( d->mURL );
		GVImageFrames frames;
		GVCache::instance()->getFrames( d->mURL, frames, format );
		if( !frames.isEmpty()) {
			d->mImageFormat = format;
			d->mFrames = frames;
			if( !d->mRawData.isNull() || format != "JPEG" ) {
				finish( true );
				return;
			}
			// the raw data is needed for JPEG, so it needs to be loaded
			// if it's not in the cache
		} else {
			// Image in cache is broken, let's try the file
			if( !d->mRawData.isNull()) {
				d->mTimeSinceLastUpdate.start();
				d->mDecoderTimer.start(0, false);
				return;
			}
		}
	}
	
	// Start loading the image
	d->mTimestamp = urlTimestamp;
	KIO::Job* getJob=KIO::get( d->mURL, false, false);

	connect(getJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
		this, SLOT(slotDataReceived(KIO::Job*, const QByteArray&)) );
	
	connect(getJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotGetResult(KIO::Job*)) );

	d->mRawData.resize(0);
	d->mTimeSinceLastUpdate.start();
}
	

void GVImageLoader::slotGetResult(KIO::Job* job) {
	LOG("error code: " << job->error());
	if( job->error() != 0 ) {
		// failed
		finish( false );
		return;
	}

	d->mGetComplete = true;
	
	// Store raw data in cache
	// Note: GVCache will give high cost to non-JPEG raw data.
	GVCache::instance()->addFile( d->mURL, d->mRawData, d->mTimestamp );

	if( !d->mImageFormat.isNull()) { // image was in cache, but not raw data
		finish( true );
		return;
	}

	// Start the decoder thread if needed
	if( d->mUseThread ) {
		startThread();
	} else { // Finish decoding if needed
		if( !d->mDecoderTimer.isActive()) d->mDecoderTimer.start(0);
	}
}


void GVImageLoader::slotDataReceived(KIO::Job*, const QByteArray& chunk) {
	LOG("size: " << chunk.size());
	if (chunk.size()<=0) return;

	int oldSize=d->mRawData.size();
	d->mRawData.resize(oldSize + chunk.size());
	memcpy(d->mRawData.data()+oldSize, chunk.data(), chunk.size() );

	// Decode the received data
	if( !d->mDecoderTimer.isActive() && !d->mUseThread) {
		d->mDecoderTimer.start(0);
	}
}


void GVImageLoader::decodeChunk() {
	if( d->mSuspended ) {
		d->mDecoderTimer.stop();
		return;
	}
	if( !d->mImageFormat.isNull()) { // image was in cache, only loading the raw data
		d->mDecoderTimer.stop();
		return;
	}

	int chunkSize = QMIN(DECODE_CHUNK_SIZE, int(d->mRawData.size())-d->mDecodedSize);
	LOG("chunkSize: " << chunkSize);
	int decodedSize = 0;
	if (chunkSize>0) {
		decodedSize = d->mDecoder.decode(
			(const uchar*)(d->mRawData.data()+d->mDecodedSize),
			chunkSize);
		LOG("decodedSize: " << decodedSize);

		if (decodedSize<0) {
			// We can't use async decoding, switch to decoder thread 
			d->mDecoderTimer.stop();
			d->mUseThread=true;
			if( d->mGetComplete ) startThread();	
			return;
		}

		// We just decoded some data
		d->mDecodedSize+=decodedSize;
	}

	if (decodedSize == 0) {
		// We decoded as much as possible from the buffer, wait to receive
		// more data before coming again in decodeChunk
		d->mDecoderTimer.stop();
		if (d->mGetComplete && !d->mAsyncImageComplete) {
			// No more data is available, the image must be truncated,
			// let's simulate its end
			end();
		}
	}
}


void GVImageLoader::startThread() {
	LOG("starting decoder thread");
	d->mDecoderThread.setRawData(d->mRawData);
	d->mDecoderThread.start();
}


void GVImageLoader::slotDecoderThreadFailed() {
	LOG("");
	// Image can't be loaded
	finish( false );
}


void GVImageLoader::slotImageDecoded() {
	LOG("");

	// Get image
	if (d->mUseThread) {
		d->mFrames.clear();
		d->mFrames.append( GVImageFrame( d->mDecoderThread.popLoadedImage(), 0 ));
	} else if( d->mFrames.count() == 0 ) {
		d->mFrames.append( GVImageFrame( d->mDecoder.image(), 0 ));
	}
	
	// Set image format
	QBuffer buffer(d->mRawData);
	buffer.open(IO_ReadOnly);
	d->mImageFormat = QImageIO::imageFormat(&buffer);
	buffer.close();

	finish( true );
}


/**
 * Make the final adjustments to the image.
 */
void GVImageLoader::finish( bool ok ) {
	LOG("");

	d->mDecodeComplete = true;

	if( !ok || d->mFrames.count() == 0 ) {
		d->mFrames.clear();
		d->mRawData = QByteArray();
		d->mImageFormat = QCString();
		d->mProcessedImage = QImage();
		emit imageLoaded( false );
		return;
	}

	GVCache::instance()->addImage( d->mURL, d->mFrames, d->mImageFormat, d->mTimestamp );

	GVImageFrame lastframe = d->mFrames.last();
	d->mFrames.pop_back(); // maintain that processedImage() is not included when calling imageChanged()
	d->mProcessedImage = lastframe.image;
	// The decoder did not cause some signals to be emitted, let's do it now
	if (d->mKnownSize.isEmpty()) {
		emit sizeLoaded( lastframe.image.width(), lastframe.image.height());
	}
	if (!d->mLoadChangedRect.isEmpty()) {
		emit imageChanged( d->mLoadChangedRect );
	} else if (!d->mUpdatedDuringLoad) {
		emit imageChanged( QRect(QPoint(0,0), lastframe.image.size()) );
	}
	d->mFrames.push_back( lastframe );

	emit imageLoaded( true );
}


void GVImageLoader::slotBusyLevelChanged( GVBusyLevel level ) {
	if( level > BUSY_LOADING ) {
		suspendLoading();
	} else {
		resumeLoading();
	}
}

void GVImageLoader::suspendLoading() {
	d->mDecoderTimer.stop();
	d->mSuspended = true;
}

void GVImageLoader::resumeLoading() {
	d->mSuspended = false;
	d->mDecoderTimer.start(0, false);
}


//---------------------------------------------------------------------
//
// QImageConsumer
//
//---------------------------------------------------------------------
void GVImageLoader::end() {
	LOG("");

	d->mDecoderTimer.stop();
	d->mAsyncImageComplete=true;
	
	// The image has been totally decoded, we delay the call to finish because
	// when we return from this function we will be in decodeChunk(), after the
	// call to decode(), so we don't want to switch to a new impl yet, since
	// this means deleting "this".
	QTimer::singleShot(0, this, SLOT(slotImageDecoded()) );
}

void GVImageLoader::changed(const QRect& rect) {
	d->mProcessedImage = d->mDecoder.image();
	d->mWasFrameData = true;
	d->mUpdatedDuringLoad=true;
	d->mLoadChangedRect |= rect;
	d->mLoadedRegion |= rect;
	if( d->mTimeSinceLastUpdate.elapsed() > 200 ) {
		LOG(d->mLoadChangedRect.left() << "-" << d->mLoadChangedRect.top()
			<< " " << d->mLoadChangedRect.width() << "x" << d->mLoadChangedRect.height() );
		emit imageChanged(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
		d->mTimeSinceLastUpdate.start();
	}
}

void GVImageLoader::frameDone() {
	frameDone( QPoint( 0, 0 ), QRect( 0, 0, d->mDecoder.image().width(), d->mDecoder.image().height()));
}

void GVImageLoader::frameDone(const QPoint& offset, const QRect& rect) {
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
	QImage image = d->mDecoder.image();
	image.detach();
	if( offset != QPoint( 0, 0 ) || rect != QRect( 0, 0, image.width(), image.height())) {
                if( !d->mFrames.isEmpty()) { // not first image - what to do here?
			QImage im( d->mFrames.last().image);
			im.detach();
			bitBlt( &im, offset.x(), offset.y(), &image, rect.x(), rect.y(), rect.width(), rect.height());
			image = im;
		}
	}
	d->mFrames.append( GVImageFrame( image, d->mNextFrameDelay ));
	d->mNextFrameDelay = 0;
	emit frameLoaded();
}

void GVImageLoader::setLooping(int) {
}

void GVImageLoader::setFramePeriod(int milliseconds) {
	if( milliseconds < 0 ) milliseconds = 0; // -1 means showing immediately
	if( d->mNextFrameDelay == 0 || milliseconds != 0 ) {
		d->mNextFrameDelay = milliseconds;
	}
}

void GVImageLoader::setSize(int width, int height) {
	LOG(width << "x" << height);
	d->mKnownSize = QSize( width, height );
	// FIXME: There must be a better way than creating an empty image
	d->mProcessedImage = QImage( width, height, 32 );
	emit sizeLoaded(width, height);
}


QImage GVImageLoader::processedImage() const {
	return d->mProcessedImage;
}


QSize GVImageLoader::knownSize() const {
	return d->mKnownSize;
}


GVImageFrames GVImageLoader::frames() const {
	return d->mFrames;
}


QCString GVImageLoader::imageFormat() const {
	return d->mImageFormat;
}


QByteArray GVImageLoader::rawData() const {
	return d->mRawData;
}


KURL GVImageLoader::url() const {
	return d->mURL;
}


QRegion GVImageLoader::loadedRegion() const {
	return d->mLoadedRegion;
}


bool GVImageLoader::completed() const {
	return d->mDecodeComplete;
}


void GVImageLoader::ref() {
	++d->mRefcount;
}

void GVImageLoader::deref() {
	if( --d->mRefcount <= 0 ) {
		loaders.remove( d->mURL );
		delete this;
	}
}

void GVImageLoader::release() {
	deref();
}

//---------------------------------------------------------------------
//
// Managing loaders
//
//---------------------------------------------------------------------

GVImageLoader* GVImageLoader::loader( const KURL& url ) {
	if( loaders.contains( url )) {
		GVImageLoader* l = loaders[ url ];
		l->ref();
		return l;
	}
	GVImageLoader* l = new GVImageLoader;
	l->ref();
	loaders[ url ] = l;
	l->startLoading( url );
	return l;
}
