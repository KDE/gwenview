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

// Qt
#include <qbuffer.h>
#include <qfile.h>
#include <qguardedptr.h>
#include <qimage.h>
#include <qmemarray.h>
#include <qstring.h>
#include <qtimer.h>
#include <qdatetime.h>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <kurl.h>

// Local
#include "gvdocumentloadedimpl.h"
#include "gvdocumentjpegloadedimpl.h"
#include "gvdocumentanimatedloadedimpl.h"
#include "gvdocumentdecodeimpl.moc"
#include "gvcache.h"
#include "gvimageframe.h"

const unsigned int DECODE_CHUNK_SIZE=4096;

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

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
// GVDocumentDecodeImplPrivate
//
//---------------------------------------------------------------------
class GVDocumentDecodeImplPrivate {
public:
	GVDocumentDecodeImplPrivate(GVDocumentDecodeImpl* impl)
	: mDecodedSize(0)
	, mUseThread(false)
	, mDecoder(impl)
	, mUpdatedDuringLoad(false)
	, mSuspended(false)
	, mGetComplete(false)
	, mAsyncImageComplete(false)
	, mNextFrameDelay(0)
	, mWasFrameData(false)
	{}

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
	
	// Set to true if at least one rectUpdated signals have been emitted
	bool mUpdatedDuringLoad;

	// A rect of recently loaded pixels that the rest of the application has
	// not been notified about with the rectUpdated signal
	QRect mLoadChangedRect; 
	
	// The time since we last emitted the rectUpdated signal
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

	GVImageFrames mFrames;
};


//---------------------------------------------------------------------
//
// GVDocumentDecodeImpl
//
//---------------------------------------------------------------------
GVDocumentDecodeImpl::GVDocumentDecodeImpl(GVDocument* document) 
: GVDocumentImpl(document) {
	LOG("");
	d=new GVDocumentDecodeImplPrivate(this);

	connect(&d->mDecoderTimer, SIGNAL(timeout()), this, SLOT(decodeChunk()) );

	connect(&d->mDecoderThread, SIGNAL(succeeded()),
		this, SLOT(slotImageDecoded()) );
	connect(&d->mDecoderThread, SIGNAL(failed()),
		this, SLOT(slotDecoderThreadFailed()) );
	
	QTimer::singleShot(0, this, SLOT(start()) );
}


GVDocumentDecodeImpl::~GVDocumentDecodeImpl() {
	LOG("");
	if (d->mDecoderThread.running()) {
		d->mDecoderThread.cancel();
		d->mDecoderThread.wait();
	}
	delete d;
}


void GVDocumentDecodeImpl::start() {
	d->mLoadChangedRect=QRect();
	d->mTimestamp = GVCache::instance()->timestamp( mDocument->url() );
	d->mFrames.clear();
	d->mWasFrameData = false;
	d->mNextFrameDelay = 0;
	KIO::Job* job=KIO::stat( mDocument->url(), false);
	connect(job, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotStatResult(KIO::Job*)) );
}

void GVDocumentDecodeImpl::slotStatResult(KIO::Job* job) {
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

	if( urlTimestamp <= d->mTimestamp ) {
		// We have the image in cache
		QCString format;
		d->mRawData = GVCache::instance()->file( mDocument->url() );
		GVImageFrames frames = GVCache::instance()->frames( mDocument->url(), format );
		if( !frames.isEmpty()) {
			setImageFormat(format);
			d->mFrames = frames;
			finish();
			return;
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
	KIO::Job* getJob=KIO::get( mDocument->url(), false, false);

	connect(getJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
		this, SLOT(slotDataReceived(KIO::Job*, const QByteArray&)) );
	
	connect(getJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotGetResult(KIO::Job*)) );

	d->mRawData.resize(0);
	d->mTimeSinceLastUpdate.start();
}
	

void GVDocumentDecodeImpl::slotGetResult(KIO::Job* job) {
	LOG("error code: " << job->error());
	if( job->error() != 0 ) {
		// failed
		emit finished(false);
		switchToImpl(new GVDocumentImpl(mDocument));
		return;
	}

	d->mGetComplete = true;
	
	// Start the decoder thread if needed
	if( d->mUseThread ) {
		startThread();
	} else { // Finish decoding if needed
		if( !d->mDecoderTimer.isActive()) d->mDecoderTimer.start(0);
	}
}


void GVDocumentDecodeImpl::slotDataReceived(KIO::Job*, const QByteArray& chunk) {
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


void GVDocumentDecodeImpl::decodeChunk() {
	if( d->mSuspended ) {
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


void GVDocumentDecodeImpl::startThread() {
	LOG("starting decoder thread");
	d->mDecoderThread.setRawData(d->mRawData);
	d->mDecoderThread.start();
}


void GVDocumentDecodeImpl::slotDecoderThreadFailed() {
	LOG("");
	// Image can't be loaded, let's switch to an empty implementation
	emit finished(false);
	switchToImpl(new GVDocumentImpl(mDocument));
}


void GVDocumentDecodeImpl::slotImageDecoded() {
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
	const char* format = QImageIO::imageFormat(&buffer);
	buffer.close();

	if (!format) {
		// Unknown format, no need to go further
		emit finished(false);
		switchToImpl(new GVDocumentImpl(mDocument));
		return;
	}
	setImageFormat( format );
	
	// Store raw data in cache
	GVCache::instance()->addFile( mDocument->url(), d->mRawData, d->mTimestamp );
	
	finish();
}


/**
 * Make the final adjustments to the image before switching to a loaded
 * implementation
 */
void GVDocumentDecodeImpl::finish() {
	LOG("");

	QImage image = d->mFrames.first().image;

	GVCache::instance()->addImage( mDocument->url(), d->mFrames, mDocument->imageFormat(), d->mTimestamp );

	// The decoder did not cause the sizeUpdated or rectUpdated signals to be
	// emitted, let's do it now
	if (!d->mUpdatedDuringLoad) {
		setImage(image);
		emit sizeUpdated(image.width(), image.height());
		emit rectUpdated( QRect(QPoint(0,0), image.size()) );
	}

	// Update file info
	setFileSize(d->mRawData.size());
	
	// Now we switch to a loaded implementation
	if ( d->mFrames.count() > 1 ) {
		switchToImpl( new GVDocumentAnimatedLoadedImpl(mDocument, d->mFrames));
	} else if (qstrcmp(mDocument->imageFormat(), "JPEG")==0) {
		// We want a local copy of the file for the comment editor
		QString tempFilePath;
		if (!mDocument->url().isLocalFile()) {
			KTempFile tempFile;
			tempFile.dataStream()->writeRawBytes(d->mRawData.data(), d->mRawData.size());
			tempFile.close();
			tempFilePath=tempFile.name();
		}
		switchToImpl(new GVDocumentJPEGLoadedImpl(mDocument, d->mRawData, tempFilePath));
	} else {
		switchToImpl(new GVDocumentLoadedImpl(mDocument));
	}
}


void GVDocumentDecodeImpl::suspendLoading() {
	d->mDecoderTimer.stop();
	d->mSuspended = true;
}

void GVDocumentDecodeImpl::resumeLoading() {
	d->mSuspended = false;
	d->mDecoderTimer.start(0, false);
}


//---------------------------------------------------------------------
//
// QImageConsumer
//
//---------------------------------------------------------------------
void GVDocumentDecodeImpl::end() {
	LOG("");
	if( !d->mLoadChangedRect.isNull() && d->mFrames.count() == 0 ) {
		emit rectUpdated(d->mLoadChangedRect);
	}

	d->mDecoderTimer.stop();
	d->mAsyncImageComplete=true;
	
	// The image has been totally decoded, we delay the call to finish because
	// when we return from this function we will be in decodeChunk(), after the
	// call to decode(), so we don't want to switch to a new impl yet, since
	// this means deleting "this".
	QTimer::singleShot(0, this, SLOT(slotImageDecoded()) );
}

void GVDocumentDecodeImpl::changed(const QRect& rect) {
	d->mWasFrameData = true;
	if( d->mFrames.count() > 0 ) return;
	if (!d->mUpdatedDuringLoad) {
		setImage(d->mDecoder.image());
		d->mUpdatedDuringLoad=true;
	}
	d->mLoadChangedRect |= rect;
	if( d->mTimeSinceLastUpdate.elapsed() > 200 ) {
		LOG(d->mLoadChangedRect.left() << "-" << d->mLoadChangedRect.top()
			<< " " << d->mLoadChangedRect.width() << "x" << d->mLoadChangedRect.height() );
		emit rectUpdated(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
		d->mTimeSinceLastUpdate.start();
	}
}

void GVDocumentDecodeImpl::frameDone() {
	frameDone( QPoint( 0, 0 ), QRect( 0, 0, d->mDecoder.image().width(), d->mDecoder.image().height()));
}

void GVDocumentDecodeImpl::frameDone(const QPoint& offset, const QRect& rect) {
	// Another case where the image loading in Qt's is a bit borken.
	// It's possible to get several notes about a frame being done for one frame (with MNG).
	if( !d->mWasFrameData ) return;
	d->mWasFrameData = false;
	if( !d->mLoadChangedRect.isNull() && d->mFrames.count() == 0 ) {
		emit rectUpdated(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
		d->mTimeSinceLastUpdate.start();
	}
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
	if( d->mFrames.count() == 0 ) {
		setImage( image ); // explicit sharing - don't modify the image in document anymore
	}
	d->mFrames.append( GVImageFrame( image, d->mNextFrameDelay ));
	d->mNextFrameDelay = 0;
}

void GVDocumentDecodeImpl::setLooping(int) {
}

void GVDocumentDecodeImpl::setFramePeriod(int milliseconds) {
	if( milliseconds < 0 ) milliseconds = 0; // -1 means showing immediately
	if( d->mNextFrameDelay == 0 || milliseconds != 0 ) {
		d->mNextFrameDelay = milliseconds;
	}
}

void GVDocumentDecodeImpl::setSize(int width, int height) {
	LOG(width << "x" << height);
	// FIXME: There must be a better way than creating an empty image
	setImage(QImage(width, height, 32));
	emit sizeUpdated(width, height);
}

