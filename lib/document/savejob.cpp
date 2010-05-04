// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "savejob.h"

// Qt
#include <QFuture>
#include <QFutureWatcher>
#include <QScopedPointer>
#include <QtConcurrentRun>

// KDE
#include <kapplication.h>
#include <kio/copyjob.h>
#include <kio/jobuidelegate.h>
#include <klocale.h>
#include <ksavefile.h>
#include <ktemporaryfile.h>
#include <kurl.h>

// Local
#include "documentloadedimpl.h"

namespace Gwenview {


struct SaveJobPrivate {
	DocumentLoadedImpl* mImpl;
	KUrl mUrl;
	QByteArray mFormat;
	QScopedPointer<KTemporaryFile> mTemporaryFile;
	QScopedPointer<KSaveFile> mSaveFile;
};


SaveJob::SaveJob(DocumentLoadedImpl* impl, const KUrl& url, const QByteArray& format)
: d(new SaveJobPrivate) {
	d->mImpl = impl;
	d->mUrl = url;
	d->mFormat = format;
}


SaveJob::~SaveJob() {
	delete d;
}


void SaveJob::saveInternal() {
	if (!d->mImpl->saveInternal(d->mSaveFile.data(), d->mFormat)) {
		d->mSaveFile->abort();
		setError(UserDefinedError + 2);
		return;
	}

	if (!d->mSaveFile->finalize()) {
		setErrorText(i18nc("@info", "Could not overwrite file, check that you have the necessary rights to write in <filename>%1</filename>.", d->mUrl.pathOrUrl()));
		setError(UserDefinedError + 3);
	}
}


void SaveJob::doStart() {
	QString fileName;

	if (d->mUrl.isLocalFile()) {
		fileName = d->mUrl.toLocalFile();
	} else {
		d->mTemporaryFile.reset(new KTemporaryFile);
		d->mTemporaryFile->setAutoRemove(true);
		d->mTemporaryFile->open();
		fileName = d->mTemporaryFile->fileName();
	}

	d->mSaveFile.reset(new KSaveFile(fileName));

	if (!d->mSaveFile->open()) {
		KUrl dirUrl = d->mUrl;
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

	if (d->mUrl.isLocalFile()) {
		emitResult();
	} else {
		KIO::Job* job = KIO::copy(KUrl::fromPath(d->mTemporaryFile->fileName()), d->mUrl);
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


} // namespace
