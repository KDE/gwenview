// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "loadingdocumentimpl.moc"

// Qt
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

// KDE
#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kmountpoint.h>
#include <kurl.h>

// Local
#include "document.h"
#include "documentloadedimpl.h"
#include "emptydocumentimpl.h"
#include "imageutils.h"
#include "jpegcontent.h"
#include "jpegdocumentloadedimpl.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

/**
 * CancellableBuffer
 * This class acts like QBuffer, but will simulates a truncated file if the
 * TSThread which was passed to its constructor has been asked for cancellation
 */
class CancellableBuffer : public QBuffer {
public:
	CancellableBuffer()
	: mCancel(false)
	{}

	void cancel() {
		QMutexLocker lock(&mMutex);
		mCancel = true;
	}

	bool atEnd() const {
		if (testCancel()) {
			LOG("cancel detected");
			return true;
		}
		return QBuffer::atEnd();
	}

	qint64 bytesAvailable() const {
		if (testCancel()) {
			LOG("cancel detected");
			return 0;
		}
		return QBuffer::bytesAvailable();
	}

	bool canReadLine() const {
		if (testCancel()) {
			LOG("cancel detected");
			return 0;
		}
		return QBuffer::canReadLine();
	}

	qint64 readData(char * data, qint64 maxSize) {
		if (testCancel()) {
			LOG("cancel detected");
			return 0;
		}
		return QBuffer::readData(data, maxSize);
	}

	qint64 readLineData(char * data, qint64 maxSize) {
		if (testCancel()) {
			LOG("cancel detected");
			return 0;
		}
		return QBuffer::readLineData(data, maxSize);
	}

private:
	bool testCancel() const {
		QMutexLocker lock(&mMutex);
		return mCancel;
	}

	mutable QMutex mMutex;
	bool mCancel;
};


class LoadingThread : public QThread {
public:
	LoadingThread()
	: mJpegContent(0) {
	}

	~LoadingThread() {
		delete mJpegContent;
	}

	virtual void run() {
		QMutexLocker lock(&mMutex);
		mBuffer.setBuffer(&mData);
		mBuffer.open(QIODevice::ReadOnly);
		mFormat = QImageReader::imageFormat(&mBuffer);

		bool ok = mImage.load(&mBuffer, mFormat.data());
		if (!ok) {
			return;
		}
		if (mFormat == "jpeg") {
			mJpegContent = new JpegContent();
			if (!mJpegContent->loadFromData(mData)) {
				return;
			}
			Gwenview::Orientation orientation = mJpegContent->orientation();
			QMatrix matrix = ImageUtils::transformMatrix(orientation);
			mImage = mImage.transformed(matrix);
		}
	}

	void cancel() {
		mBuffer.cancel();
	}

	void setData(const QByteArray& data) {
		QMutexLocker lock(&mMutex);
		mData = data;
	}

	const QByteArray& format() const {
		QMutexLocker lock(&mMutex);
		return mFormat;
	}

	const QImage& image() const {
		QMutexLocker lock(&mMutex);
		return mImage;
	}

	JpegContent* popJpegContent() {
		QMutexLocker lock(&mMutex);
		JpegContent* tmp = mJpegContent;
		mJpegContent = 0;
		return tmp;
	}

private:
	mutable QMutex mMutex;
	CancellableBuffer mBuffer;
	QByteArray mData;
	QByteArray mFormat;
	QImage mImage;
	JpegContent* mJpegContent;
};


struct LoadingDocumentImplPrivate {
	LoadingDocumentImpl* mImpl;
	QPointer<KIO::TransferJob> mTransferJob;
	QByteArray mData;
	LoadingThread mThread;

	void startLoadingThread() {
		mThread.setData(mData);
		QObject::connect(&mThread, SIGNAL(finished()), mImpl, SLOT(slotImageLoaded()) );
		mThread.start();
	}
};


LoadingDocumentImpl::LoadingDocumentImpl(Document* document)
: AbstractDocumentImpl(document)
, d(new LoadingDocumentImplPrivate) {
	d->mImpl = this;
}


LoadingDocumentImpl::~LoadingDocumentImpl() {
	LOG("");
	if (d->mTransferJob) {
		d->mTransferJob->kill();
	}
	if (d->mThread.isRunning()) {
		LOG("");
		d->mThread.cancel();
		d->mThread.wait();
	}
	delete d;
}

void LoadingDocumentImpl::init() {
	KUrl url = document()->url();
	// Check if we should open directly
	KMountPoint::List mpl = KMountPoint::currentMountPoints();
	KMountPoint::Ptr mp = mpl.findByPath( url.path() );

	if (url.isLocalFile() && !mp->probablySlow()) {
		// Load file content directly
		QFile file(url.path());
		if (!file.open(QIODevice::ReadOnly)) {
			kWarning() << "Couldn't open" << url;
			switchToImpl(new EmptyDocumentImpl(document()));
			return;
		}
		d->mData = file.readAll();
		d->startLoadingThread();
	} else {
		// Transfer file via KIO
		d->mTransferJob = KIO::get(document()->url());
		connect(d->mTransferJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
			SLOT(slotDataReceived(KIO::Job*, const QByteArray&)) );
		connect(d->mTransferJob, SIGNAL(result(KJob*)),
			SLOT(slotTransferFinished(KJob*)) );
		d->mTransferJob->start();
	}
}


void LoadingDocumentImpl::slotDataReceived(KIO::Job*, const QByteArray& chunk) {
	d->mData.append(chunk);
}


void LoadingDocumentImpl::slotTransferFinished(KJob* job) {
	if (job->error()) {
		//FIXME: Better error handling
		kWarning() << job->errorString();
		switchToImpl(new EmptyDocumentImpl(document()));
		return;
	}
	d->startLoadingThread();
}


bool LoadingDocumentImpl::isLoaded() const {
	return false;
}


void LoadingDocumentImpl::slotImageLoaded() {
	Q_ASSERT(d->mThread.isFinished());
	setDocumentImage(d->mThread.image());
	loaded();
	QByteArray format = d->mThread.format();
	setDocumentFormat(format);
	if (format == "jpeg") {
		JpegDocumentLoadedImpl* impl = new JpegDocumentLoadedImpl(
			document(),
			d->mThread.popJpegContent());
		switchToImpl(impl);
	} else {
		switchToImpl(new DocumentLoadedImpl(document()));
	}
}


Document::SaveResult LoadingDocumentImpl::save(const KUrl&, const QByteArray&) {
	return Document::SR_OtherError;
}

void LoadingDocumentImpl::setImage(const QImage&) {
	kWarning() << " should not be called\n";
}

} // namespace
