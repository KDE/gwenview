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
#include <kdebug.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <kurl.h>

// Local
#include "gvdocumentloadedimpl.h"
#include "gvdocumentjpegloadedimpl.h"
#include "gvdocumentdecodeimpl.moc"
#include "gvcache.h"

const unsigned int DECODE_CHUNK_SIZE=4096;


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
			kdDebug() << k_funcinfo << "cancel detected" << endl;
			return true;
		}
		return QBuffer::atEnd();
	}
	
	Q_LONG readBlock(char * data, Q_ULONG maxlen) {
		if (mThread->testCancel()) {
			kdDebug() << k_funcinfo << "cancel detected" << endl;
			return 0;
		}
		return QBuffer::readBlock(data, maxlen);
	}

	Q_LONG readLine(char * data, Q_ULONG maxlen) {
		if (mThread->testCancel()) {
			kdDebug() << k_funcinfo << "cancel detected" << endl;
			return 0;
		}
		return QBuffer::readLine(data, maxlen);
	}

	QByteArray readAll() {
		if (mThread->testCancel()) {
			kdDebug() << k_funcinfo << "cancel detected" << endl;
			return QByteArray();
		}
		return QBuffer::readAll();
	}

	int getch() {
		if (mThread->testCancel()) {
			kdDebug() << k_funcinfo << "cancel detected" << endl;
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
	kdDebug() << k_funcinfo << endl;
	
	// This block makes sure imageIO won't access the image after the signal
	// has been posted
	{
		QImageIO imageIO;
		
		GVCancellableBuffer buffer(mRawData, this);
		buffer.open(IO_ReadOnly);
		imageIO.setIODevice(&buffer);
		bool ok=imageIO.read();
		if (testCancel()) {
			kdDebug() << k_funcinfo << "cancelled" << endl;
			return;
		}
			
		if (!ok) {
			kdDebug() << k_funcinfo << " failed" << endl;
			postSignal( SIGNAL(failed()) );
			return;
		}
		
		kdDebug() << k_funcinfo << " succeeded" << endl;
		mImage=imageIO.image();
	}
	
	kdDebug() << k_funcinfo << " succeeded, emitting signal" << endl;
	postSignal( SIGNAL(succeeded()) );
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
	: mReadSize( 0 ), mDecoder(impl), mSuspended( false ) {}

	bool mUpdatedDuringLoad;
	QByteArray mRawData;
	unsigned int mReadSize;
	QImageDecoder mDecoder;
	QTimer mDecoderTimer;
	QRect mLoadChangedRect;
	QTime mTimeSinceLastUpdate;
	QGuardedPtr<KIO::Job> mJob;
	bool mSuspended;
	QDateTime mTimestamp;
	bool mUseThread;
	GVDecoderThread mDecoderThread;
};


//---------------------------------------------------------------------
//
// GVDocumentDecodeImpl
//
//---------------------------------------------------------------------
GVDocumentDecodeImpl::GVDocumentDecodeImpl(GVDocument* document) 
: GVDocumentImpl(document) {
	kdDebug() << k_funcinfo << endl;
	d=new GVDocumentDecodeImplPrivate(this);
	d->mUpdatedDuringLoad=false;

	connect(&d->mDecoderTimer, SIGNAL(timeout()), this, SLOT(decodeChunk()) );

	connect(&d->mDecoderThread, SIGNAL(succeeded()),
		this, SLOT(slotImageDecoded()) );
	connect(&d->mDecoderThread, SIGNAL(failed()),
		this, SLOT(slotDecoderThreadFailed()) );
	
	QTimer::singleShot(0, this, SLOT(start()) );
}


GVDocumentDecodeImpl::~GVDocumentDecodeImpl() {
	if (d->mDecoderThread.running()) {
		d->mDecoderThread.cancel();
		d->mDecoderThread.wait();
	}
	delete d;
}

void GVDocumentDecodeImpl::start() {
	d->mLoadChangedRect=QRect();
	d->mTimestamp = GVCache::instance()->timestamp( mDocument->url() );
	d->mJob=KIO::stat( mDocument->url(), false);
	connect(d->mJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotStatResult(KIO::Job*)) );
}

void GVDocumentDecodeImpl::slotStatResult(KIO::Job*job) {
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
		QCString format;
		QImage image = GVCache::instance()->image( mDocument->url(), format );
		if( !image.isNull()) {
			setImageFormat(format);
			finish(image);
			return;
		} else {
			QByteArray data = GVCache::instance()->file( mDocument->url() );
			if( !data.isNull()) {
				d->mJob = NULL;
				d->mRawData = data;
				d->mReadSize=0;
				d->mUseThread = false;
				d->mTimeSinceLastUpdate.start();
				d->mDecoderTimer.start(0, false);
				return;
			}
		}
	}
	d->mTimestamp = urlTimestamp;
	startLoading();
}

void GVDocumentDecodeImpl::startLoading() {
	d->mJob=KIO::get( mDocument->url(), false, false);

	connect(d->mJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
		this, SLOT(slotDataReceived(KIO::Job*, const QByteArray&)) );
	
	connect(d->mJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotResult(KIO::Job*)) );

	d->mRawData.resize(0);
	d->mReadSize=0;
	d->mUseThread = false;
	d->mTimeSinceLastUpdate.start();
}
	

void GVDocumentDecodeImpl::slotResult(KIO::Job* job) {
	kdDebug() << k_funcinfo << "error code: " << job->error() << endl;
	if( job->error() != 0 ) {
		// failed
		emit finished(false);
		switchToImpl(new GVDocumentImpl(mDocument));
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
	
	// Start the decoder thread if needed
	if( d->mUseThread ) {
		kdDebug() << k_funcinfo << "starting decoder thread\n";
		d->mDecoderThread.setRawData(d->mRawData);
		d->mDecoderThread.start();
	}
}


void GVDocumentDecodeImpl::slotDataReceived(KIO::Job*, const QByteArray& chunk) {
	kdDebug() << k_funcinfo << "size: " << chunk.size() << endl;
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

	int chunkSize = QMIN(DECODE_CHUNK_SIZE, int(d->mRawData.size())-d->mReadSize);
	//kdDebug() << k_funcinfo << "chunkSize: " << chunkSize << endl;
	Q_ASSERT(chunkSize>0);
	if (chunkSize<=0) return;
		
	int decodedSize = d->mDecoder.decode(
		(const uchar*)(d->mRawData.data()+d->mReadSize),
		chunkSize);
	//kdDebug() << k_funcinfo << "decodedSize: " << decodedSize << endl;
	
	if (decodedSize<0) {
		// We can't use async decoding, switch to decoder thread 
		d->mDecoderTimer.stop();
		d->mUseThread=true;
		return;
	}

	if (decodedSize>0) {
		// We just decoded some data
		d->mReadSize+=decodedSize;
		if (d->mReadSize==d->mRawData.size()) {
			// We decoded the whole buffer, wait to receive more data
			// before coming again in decodeChunk
			d->mDecoderTimer.stop();
		}
	}
}



void GVDocumentDecodeImpl::slotDecoderThreadFailed() {
	kdDebug() << k_funcinfo << endl;
	// Image can't be loaded, let's switch to an empty implementation
	emit finished(false);
	switchToImpl(new GVDocumentImpl(mDocument));
}


/**
 * This is just a simple slot which calls finish with the appropriate image
 */
void GVDocumentDecodeImpl::slotImageDecoded() {
	kdDebug() << k_funcinfo << endl;
	QImage image;
	if (d->mUseThread) {
		image=d->mDecoderThread.popLoadedImage();
	} else {
		image=d->mDecoder.image();
	}
	finish(image);
}


/**
 * Make the final adjustments to the image before switching to a loaded
 * implementation
 */
void GVDocumentDecodeImpl::finish(QImage& image) {
	kdDebug() << k_funcinfo << endl;
	// Convert depth if necessary
	// (32 bit depth is necessary for alpha-blending)
	if (image.depth()<32 && image.hasAlphaBuffer()) {
		image=image.convertDepth(32);
		d->mUpdatedDuringLoad=false;
	}

	GVCache::instance()->addImage( mDocument->url(), image, mDocument->imageFormat(), d->mTimestamp );

	// The decoder did not cause the sizeUpdated or rectUpdated signals to be
	// emitted, let's do it now
	if (!d->mUpdatedDuringLoad) {
		setImage(image);
		emit sizeUpdated(image.width(), image.height());
		emit rectUpdated( QRect(QPoint(0,0), image.size()) );
	}
	
	// Now we switch to a loaded implementation
	if (qstrcmp(mDocument->imageFormat(), "JPEG")==0) {
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
	if(d->mReadSize < d->mRawData.size()) {
		d->mDecoderTimer.start(0, false);
	}
}


//---------------------------------------------------------------------
//
// QImageConsumer
//
//---------------------------------------------------------------------
void GVDocumentDecodeImpl::end() {
	if( !d->mLoadChangedRect.isNull()) {
		emit rectUpdated(d->mLoadChangedRect);
	}
	
	// The image has been totally decoded, we delay the call to finish because
	// when we return from this function we will be in decodeChunk(), after the
	// call to decode(), so we don't want to switch to a new impl yet, since
	// this means deleting "this".
	QImage image=d->mDecoder.image();
	d->mDecoderTimer.stop();
	QTimer::singleShot(0, this, SLOT(slotImageDecoded()) );
}

void GVDocumentDecodeImpl::changed(const QRect& rect) {
//	kdDebug() << k_funcinfo << " " << rect.left() << "-" << rect.top() << " " << rect.width() << "x" << rect.height() << endl;
	if (!d->mUpdatedDuringLoad) {
		setImage(d->mDecoder.image());
		d->mUpdatedDuringLoad=true;
	}
	d->mLoadChangedRect |= rect;
	if( d->mTimeSinceLastUpdate.elapsed() > 100 ) {
		kdDebug() << k_funcinfo << " " << d->mLoadChangedRect.left() << "-" << d->mLoadChangedRect.top()
			<< " " << d->mLoadChangedRect.width() << "x" << d->mLoadChangedRect.height() << "\n";
		emit rectUpdated(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
		d->mTimeSinceLastUpdate.start();
	}
}

void GVDocumentDecodeImpl::frameDone() {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::frameDone(const QPoint& /*offset*/, const QRect& /*rect*/) {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::setLooping(int) {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::setFramePeriod(int /*milliseconds*/) {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::setSize(int width, int height) {
	kdDebug() << k_funcinfo << " " << width << "x" << height << endl;
	// FIXME: There must be a better way than creating an empty image
	setImage(QImage(width, height, 32));
	emit sizeUpdated(width, height);
}

