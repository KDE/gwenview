// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "documentloadedimpl.h"

// Qt
#include <QByteArray>
#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QImageWriter>
#include <QMatrix>
#include <QScopedPointer>
#include <QtConcurrentRun>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <kio/copyjob.h>
#include <kio/jobuidelegate.h>
#include <klocale.h>
#include <ksavefile.h>
#include <ktemporaryfile.h>
#include <kurl.h>

// Local
#include "documentjob.h"
#include "imageutils.h"

namespace Gwenview {


struct DocumentLoadedImplPrivate {
	QByteArray mRawData;
};


DocumentLoadedImpl::DocumentLoadedImpl(Document* document, const QByteArray& rawData)
: AbstractDocumentImpl(document)
, d(new DocumentLoadedImplPrivate) {
	if (document->keepRawData()) {
		d->mRawData = rawData;
	}
}


DocumentLoadedImpl::~DocumentLoadedImpl() {
	delete d;
}


void DocumentLoadedImpl::init() {
	emit imageRectUpdated(document()->image().rect());
	emit loaded();
}


bool DocumentLoadedImpl::isEditable() const {
	return true;
}


Document::LoadingState DocumentLoadedImpl::loadingState() const {
	return Document::Loaded;
}


bool DocumentLoadedImpl::saveInternal(QIODevice* device, const QByteArray& format) {
	QImageWriter writer(device, format);
	bool ok = writer.write(document()->image());
	if (ok) {
		setDocumentFormat(format);
	} else {
		setDocumentErrorString(writer.errorString());
	}
	return ok;
}

SaveJob::SaveJob(DocumentLoadedImpl* impl, const KUrl& url, const QByteArray& format)
: DocumentJob()
, mImpl(impl)
, mUrl(url)
, mFormat(format)
{}

SaveJob::~SaveJob() {
}

void SaveJob::saveInternal() {
	if (!mImpl->saveInternal(mSaveFile.data(), mFormat)) {
		mSaveFile->abort();
		setError(UserDefinedError + 2);
		return;
	}

	if (!mSaveFile->finalize()) {
		setErrorText(i18nc("@info", "Could not overwrite file, check that you have the necessary rights to write in <filename>%1</filename>.", mUrl.pathOrUrl()));
		setError(UserDefinedError + 3);
	}
}

void SaveJob::doStart() {
	QString fileName;

	if (mUrl.isLocalFile()) {
		fileName = mUrl.toLocalFile();
	} else {
		mTemporaryFile.reset(new KTemporaryFile);
		mTemporaryFile->setAutoRemove(true);
		mTemporaryFile->open();
		fileName = mTemporaryFile->fileName();
	}

	mSaveFile.reset(new KSaveFile(fileName));

	if (!mSaveFile->open()) {
		KUrl dirUrl = mUrl;
		dirUrl.setFileName(QString());
		setError(UserDefinedError + 1);
		setErrorText(i18nc("@info", "Could not open file for writing, check that you have the necessary rights in <filename>%1</filename>.", dirUrl.pathOrUrl()));
		emitResult();
		return;
	}

	QFuture<void> future = QtConcurrent::run(this, &SaveJob::saveInternal);
	QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
	watcher->setFuture(future);
	connect(watcher, SIGNAL(finished()), SLOT(finishSave()));
}

void SaveJob::finishSave() {
	if (error()) {
		emitResult();
		return;
	}

	if (mUrl.isLocalFile()) {
		emitResult();
	} else {
		KIO::Job* job = KIO::copy(KUrl::fromPath(mTemporaryFile->fileName()), mUrl);
		job->ui()->setWindow(KApplication::kApplication()->activeWindow());
		addSubjob(job);
	}
}

void SaveJob::slotResult(KJob* job) {
	DocumentJob::slotResult(job);
	if (!error()) {
		emitResult();
	}
}


DocumentJob* DocumentLoadedImpl::save(const KUrl& url, const QByteArray& format) {
	return new SaveJob(this, url, format);
}


AbstractDocumentEditor* DocumentLoadedImpl::editor() {
	return this;
}


void DocumentLoadedImpl::setImage(const QImage& image) {
	setDocumentImage(image);
	imageRectUpdated(image.rect());
}


void DocumentLoadedImpl::applyTransformation(Orientation orientation) {
	QImage image = document()->image();
	QMatrix matrix = ImageUtils::transformMatrix(orientation);
	image = image.transformed(matrix);
	setDocumentImage(image);
	imageRectUpdated(image.rect());
}


QByteArray DocumentLoadedImpl::rawData() const {
	return d->mRawData;
}


} // namespace
