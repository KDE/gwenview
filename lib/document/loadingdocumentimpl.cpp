// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "loadingdocumentimpl.moc"

// Qt
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QPointer>
#include <QtConcurrentRun>

// KDE
#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kurl.h>

// Local
#include "document.h"
#include "documentloadedimpl.h"
#include "emptydocumentimpl.h"
#include "exiv2imageloader.h"
#include "imageutils.h"
#include "jpegcontent.h"
#include "jpegdocumentloadedimpl.h"
#include "orientation.h"
#include "urlutils.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif


struct LoadingDocumentImplPrivate {
	LoadingDocumentImpl* mImpl;
	Document::LoadState mLoadState;
	QPointer<KIO::TransferJob> mTransferJob;
	QFuture<bool> mMetaDataFuture;
	QFutureWatcher<bool> mMetaDataFutureWatcher;
	QFuture<void> mImageDataFuture;
	QFutureWatcher<void> mImageDataFutureWatcher;

	// State related fields
	bool mMetaDataLoaded;
	QByteArray mData;
	QByteArray mFormat;
	QSize mImageSize;
	Exiv2::Image::AutoPtr mExiv2Image;
	JpegContent* mJpegContent;
	QImage mImage;

	void startLoading() {
		Q_ASSERT(!mMetaDataLoaded);
		mMetaDataFuture = QtConcurrent::run(this, &LoadingDocumentImplPrivate::loadMetaData);
		mMetaDataFutureWatcher.setFuture(mMetaDataFuture);
	}

	bool loadMetaData() {
		QBuffer buffer;
		buffer.setBuffer(&mData);
		buffer.open(QIODevice::ReadOnly);
		QImageReader reader(&buffer);
		mFormat = reader.format();
		if (mFormat.isEmpty()) {
			return false;
		}

		Exiv2ImageLoader loader;
		if (loader.load(mData)) {
			mExiv2Image = loader.popImage();
		}

		if (mFormat == "jpeg") {
			mJpegContent = new JpegContent();
			if (!mJpegContent->loadFromData(mData, mExiv2Image.get())) {
				return false;
			}

			// Use the size from JpegContent, as its correctly transposed if the
			// image has been rotated
			mImageSize = mJpegContent->size();
		} else {
			mImageSize = reader.size();
		}

		return true;
	}

	void loadImageData() {
		QBuffer buffer;
		buffer.setBuffer(&mData);
		buffer.open(QIODevice::ReadOnly);
		QImageReader reader(&buffer);

		bool ok = reader.read(&mImage);
		if (!ok) {
			return;
		}

		if (mJpegContent) {
			Gwenview::Orientation orientation = mJpegContent->orientation();
			QMatrix matrix = ImageUtils::transformMatrix(orientation);
			mImage = mImage.transformed(matrix);
		}
	}
};


LoadingDocumentImpl::LoadingDocumentImpl(Document* document, Document::LoadState state)
: AbstractDocumentImpl(document)
, d(new LoadingDocumentImplPrivate) {
	d->mImpl = this;
	d->mMetaDataLoaded = false;
	d->mLoadState = state;
	d->mJpegContent = 0;

	connect(&d->mMetaDataFutureWatcher, SIGNAL(finished()),
		SLOT(slotMetaDataLoaded()) );

	connect(&d->mImageDataFutureWatcher, SIGNAL(finished()),
		SLOT(slotImageLoaded()) );
}


LoadingDocumentImpl::~LoadingDocumentImpl() {
	LOG("");
	if (d->mTransferJob) {
		d->mTransferJob->kill();
	}
	delete d;
}

void LoadingDocumentImpl::init() {
	KUrl url = document()->url();

	if (UrlUtils::urlIsFastLocalFile(url)) {
		// Load file content directly
		QFile file(url.path());
		if (!file.open(QIODevice::ReadOnly)) {
			kWarning() << "Couldn't open" << url;
			switchToImpl(new EmptyDocumentImpl(document()));
			return;
		}
		d->mData = file.readAll();
		d->startLoading();
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


void LoadingDocumentImpl::finishLoading() {
	Q_ASSERT(!d->mMetaDataFutureWatcher.isRunning());
	Q_ASSERT(d->mMetaDataLoaded);
	d->mLoadState = Document::LoadAll;

	d->mImageDataFuture = QtConcurrent::run(d, &LoadingDocumentImplPrivate::loadImageData);
	d->mImageDataFutureWatcher.setFuture(d->mImageDataFuture);
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
	d->startLoading();
}


bool LoadingDocumentImpl::isMetaDataLoaded() const {
	return d->mMetaDataLoaded;
}


Document::LoadingState LoadingDocumentImpl::loadingState() const {
	return Document::Loading;
}


void LoadingDocumentImpl::slotMetaDataLoaded() {
	Q_ASSERT(!d->mMetaDataFuture.isRunning());
	if (!d->mMetaDataFuture.result()) {
		kWarning() << document()->url() << "Loading metadata failed";
		switchToImpl(new EmptyDocumentImpl(document()));
		return;
	}

	setDocumentFormat(d->mFormat);
	setDocumentImageSize(d->mImageSize);
	setDocumentExiv2Image(d->mExiv2Image);

	d->mMetaDataLoaded = true;

	if (d->mLoadState == Document::LoadAll) {
		finishLoading();
	}
}


void LoadingDocumentImpl::slotImageLoaded() {
	if (!document()->size().isValid()) {
		// This can happen if the image decoder was not able to tell us the
		// image size without decoding it
		Q_ASSERT(d->mImageSize.isValid());
		setDocumentImageSize(d->mImageSize);
	}

	setDocumentImage(d->mImage);
	imageRectUpdated(d->mImage.rect());
	emit loaded();
	if (d->mFormat == "jpeg") {
		JpegDocumentLoadedImpl* impl = new JpegDocumentLoadedImpl(
			document(),
			d->mJpegContent);
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
