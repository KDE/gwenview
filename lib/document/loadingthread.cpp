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
#include "loadingthread.h"

// Qt
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QMatrix>
#include <QMutex>
#include <QMutexLocker>

// KDE

// Local
#include "imageutils.h"
#include "jpegcontent.h"
#include "orientation.h"

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


struct LoadingThreadPrivate {
	mutable QMutex mMutex;
	CancellableBuffer mBuffer;
	QByteArray mData;
	QByteArray mFormat;
	QImage mImage;
	JpegContent* mJpegContent;
};


LoadingThread::LoadingThread()
: QThread()
, d(new LoadingThreadPrivate) {
	d->mJpegContent = 0;
}


LoadingThread::~LoadingThread() {
	delete d;
}


void LoadingThread::run() {
	QMutexLocker lock(&d->mMutex);
	d->mBuffer.setBuffer(&d->mData);
	d->mBuffer.open(QIODevice::ReadOnly);
	d->mFormat = QImageReader::imageFormat(&d->mBuffer);

	bool ok = d->mImage.load(&d->mBuffer, d->mFormat.data());
	if (!ok) {
		return;
	}
	if (d->mFormat == "jpeg") {
		d->mJpegContent = new JpegContent();
		if (!d->mJpegContent->loadFromData(d->mData)) {
			return;
		}
		Gwenview::Orientation orientation = d->mJpegContent->orientation();
		QMatrix matrix = ImageUtils::transformMatrix(orientation);
		d->mImage = d->mImage.transformed(matrix);
	}
}


void LoadingThread::cancel() {
	d->mBuffer.cancel();
}


void LoadingThread::setData(const QByteArray& data) {
	QMutexLocker lock(&d->mMutex);
	d->mData = data;
}


const QByteArray& LoadingThread::format() const {
	QMutexLocker lock(&d->mMutex);
	return d->mFormat;
}


const QImage& LoadingThread::image() const {
	QMutexLocker lock(&d->mMutex);
	return d->mImage;
}


JpegContent* LoadingThread::popJpegContent() {
	QMutexLocker lock(&d->mMutex);
	JpegContent* tmp = d->mJpegContent;
	d->mJpegContent = 0;
	return tmp;
}

} // namespace
