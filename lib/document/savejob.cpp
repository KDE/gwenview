// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include <QUrl>
#include <QApplication>
#include <QTemporaryFile>
#include <QSaveFile>

// KDE
#include <KIO/CopyJob>
#include <KIO/JobUiDelegate>
#include <KLocalizedString>
#include <KJobWidgets>

// Local
#include "documentloadedimpl.h"

namespace Gwenview
{

struct SaveJobPrivate
{
    DocumentLoadedImpl* mImpl;
    QUrl mOldUrl;
    QUrl mNewUrl;
    QByteArray mFormat;
    QScopedPointer<QTemporaryFile> mTemporaryFile;
    QScopedPointer<QSaveFile> mSaveFile;
    QScopedPointer<QFutureWatcher<void> > mInternalSaveWatcher;

    bool mKillReceived;
};

SaveJob::SaveJob(DocumentLoadedImpl* impl, const QUrl &url, const QByteArray& format)
: d(new SaveJobPrivate)
{
    d->mImpl = impl;
    d->mOldUrl = impl->document()->url();
    d->mNewUrl = url;
    d->mFormat = format;
    d->mKillReceived = false;
    setCapabilities(Killable);
}

SaveJob::~SaveJob()
{
    delete d;
}

void SaveJob::saveInternal()
{
    if (!d->mImpl->saveInternal(d->mSaveFile.data(), d->mFormat)) {
        d->mSaveFile->cancelWriting();
        setError(UserDefinedError + 2);
        setErrorText(d->mImpl->document()->errorString());
    }
}

void SaveJob::doStart()
{
    if (d->mKillReceived) {
        return;
    }
    QString fileName;

    if (d->mNewUrl.isLocalFile()) {
        fileName = d->mNewUrl.toLocalFile();
    } else {
        d->mTemporaryFile.reset(new QTemporaryFile);
        d->mTemporaryFile->setAutoRemove(true);
        d->mTemporaryFile->open();
        fileName = d->mTemporaryFile->fileName();
    }

    d->mSaveFile.reset(new QSaveFile(fileName));

    if (!d->mSaveFile->open(QSaveFile::WriteOnly)) {
        QUrl dirUrl = d->mNewUrl;
        dirUrl = dirUrl.adjusted(QUrl::RemoveFilename);
        setError(UserDefinedError + 1);
        // Don't use xi18n* with markup substitution here, this is done in GvCore::slotSaveResult or SaveAllHelper::slotResult
        setErrorText(i18nc("@info", "Could not open file for writing, check that you have the necessary rights in <filename>%1</filename>.",
                           dirUrl.toDisplayString(QUrl::PreferLocalFile)));
        uiDelegate()->setAutoErrorHandlingEnabled(false);
        uiDelegate()->setAutoWarningHandlingEnabled(false);
        emitResult();
        return;
    }

    QFuture<void> future = QtConcurrent::run(this, &SaveJob::saveInternal);
    d->mInternalSaveWatcher.reset(new QFutureWatcher<void>(this));
    connect(d->mInternalSaveWatcher.data(), SIGNAL(finished()), SLOT(finishSave()));
    d->mInternalSaveWatcher->setFuture(future);
}

void SaveJob::finishSave()
{
    d->mInternalSaveWatcher.reset(nullptr);
    if (d->mKillReceived) {
        return;
    }

    if (error()) {
        emitResult();
        return;
    }

    if (!d->mSaveFile->commit()) {
        setErrorText(xi18nc("@info", "Could not overwrite file, check that you have the necessary rights to write in <filename>%1</filename>.",
                            d->mNewUrl.toString()));
        setError(UserDefinedError + 3);
        return;
    }

    if (d->mNewUrl.isLocalFile()) {
        emitResult();
    } else {
        KIO::Job* job = KIO::copy(QUrl::fromLocalFile(d->mTemporaryFile->fileName()), d->mNewUrl);
        KJobWidgets::setWindow(job, qApp->activeWindow());
        addSubjob(job);
    }
}

void SaveJob::slotResult(KJob* job)
{
    DocumentJob::slotResult(job);
    if (!error()) {
        emitResult();
    }
}

QUrl SaveJob::oldUrl() const
{
    return d->mOldUrl;
}

QUrl SaveJob::newUrl() const
{
    return d->mNewUrl;
}

bool SaveJob::doKill()
{
    d->mKillReceived = true;
    if (d->mInternalSaveWatcher) {
        d->mInternalSaveWatcher->waitForFinished();
    }
    return true;
}

} // namespace
