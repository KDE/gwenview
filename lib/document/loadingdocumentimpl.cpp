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
#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QPointer>

// KDE
#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kurl.h>

// Local
#include "document.h"
#include "documentloadedimpl.h"
#include "emptydocumentimpl.h"
#include "jpegcontent.h"
#include "jpegdocumentloadedimpl.h"
#include "loadingthread.h"
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
	bool mMetaDataLoaded;
	QPointer<KIO::TransferJob> mTransferJob;
	QByteArray mData;
	LoadingThread mThread;

	void startLoadingThread() {
		LoadingThread::Tasks tasks;
		if (!mMetaDataLoaded) {
			tasks |= LoadingThread::TaskLoadMetaData;
		}
		if (mLoadState == Document::LoadAll) {
			tasks |= LoadingThread::TaskLoadImage;
		}
		mThread.setTasks(tasks);

		mThread.start();
	}
};


LoadingDocumentImpl::LoadingDocumentImpl(Document* document, Document::LoadState state)
: AbstractDocumentImpl(document)
, d(new LoadingDocumentImplPrivate) {
	d->mImpl = this;
	d->mMetaDataLoaded = false;
	d->mLoadState = state;

	connect(&d->mThread, SIGNAL(metaDataLoaded()),
		SLOT(slotMetaDataLoaded()) );
	connect(&d->mThread, SIGNAL(sizeUpdated()),
		SLOT(slotSizeUpdated()) );
	connect(&d->mThread, SIGNAL(finished()),
		SLOT(slotFinished()) );
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

	if (UrlUtils::urlIsFastLocalFile(url)) {
		// Load file content directly
		QFile file(url.path());
		if (!file.open(QIODevice::ReadOnly)) {
			kWarning() << "Couldn't open" << url;
			switchToImpl(new EmptyDocumentImpl(document()));
			return;
		}
		d->mData = file.readAll();
		d->mThread.setData(d->mData);
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


void LoadingDocumentImpl::finishLoading() {
	d->mThread.wait();
	Q_ASSERT(d->mLoadState == Document::LoadMetaData);
	Q_ASSERT(d->mMetaDataLoaded);
	d->mLoadState = Document::LoadAll;
	d->startLoadingThread();
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
	d->mThread.setData(d->mData);
	d->startLoadingThread();
}


bool LoadingDocumentImpl::isMetaDataLoaded() const {
	return d->mMetaDataLoaded;
}


bool LoadingDocumentImpl::isLoaded() const {
	return false;
}


void LoadingDocumentImpl::slotMetaDataLoaded() {
	QByteArray format = d->mThread.format();
	QSize size = d->mThread.size();
	Exiv2::Image::AutoPtr exiv2Image = d->mThread.popExiv2Image();

	setDocumentFormat(format);
	setDocumentImageSize(size);
	setDocumentExiv2Image(exiv2Image);

	d->mMetaDataLoaded = true;
}


void LoadingDocumentImpl::slotSizeUpdated() {
	QSize size = d->mThread.size();
	setDocumentImageSize(size);
}


void LoadingDocumentImpl::slotFinished() {
	Q_ASSERT(d->mThread.isFinished());
	if (d->mLoadState == Document::LoadAll) {
		setDocumentImage(d->mThread.image());
		imageRectUpdated(d->mThread.image().rect());
		emit loaded();
		if (document()->format() == "jpeg") {
			JpegDocumentLoadedImpl* impl = new JpegDocumentLoadedImpl(
				document(),
				d->mThread.popJpegContent());
			switchToImpl(impl);
		} else {
			switchToImpl(new DocumentLoadedImpl(document()));
		}
	}
}


Document::SaveResult LoadingDocumentImpl::save(const KUrl&, const QByteArray&) {
	return Document::SR_OtherError;
}

void LoadingDocumentImpl::setImage(const QImage&) {
	kWarning() << " should not be called\n";
}

} // namespace
