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
#include "importer.h"

// Qt
#include <QDateTime>
#include <QDebug>
#include <QUrl>

// KDE
#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>
#include <KLocalizedString>

// stdc++
#include <memory>

// Local
#include <fileutils.h>
#include <filenameformater.h>
#include <lib/timeutils.h>
#include <QDir>

namespace Gwenview
{

struct ImporterPrivate
{
    Importer* q;
    QWidget* mAuthWindow;
    std::unique_ptr<FileNameFormater> mFileNameFormater;
    QUrl mTempImportDirUrl;

    /* @defgroup reset Should be reset in start()
     * @{ */
    QList<QUrl> mUrlList;
    QList<QUrl> mImportedUrlList;
    QList<QUrl> mSkippedUrlList;
    int mRenamedCount;
    int mProgress;
    int mJobProgress;
    /* @} */

    QUrl mCurrentUrl;

    void emitError(const QString& message)
    {
        QMetaObject::invokeMethod(q, "error", Q_ARG(QString, message));
    }

    bool createImportDir(const QUrl& url)
    {
        Q_ASSERT(url.isLocalFile());
        // FIXME: Support remote urls

        if (!QDir().mkpath(url.toLocalFile())) {
            emitError(i18n("Could not create destination folder."));
            return false;
        }
        QString message;
        QString dir = FileUtils::createTempDir(url.toLocalFile(), ".gwenview_importer-", &message);
        mTempImportDirUrl = QUrl::fromLocalFile(dir);
        if (mTempImportDirUrl.isEmpty()) {
            emitError(i18n("Could not create temporary upload folder:\n%1", message));
            return false;
        }
        return true;
    }

    void importNext()
    {
        if (mUrlList.empty()) {
            q->finalizeImport();
            return;
        }
        mCurrentUrl = mUrlList.takeFirst();
        QUrl dst = mTempImportDirUrl;
        dst.setPath(dst.path() + mCurrentUrl.fileName());
        KIO::Job* job = KIO::copy(mCurrentUrl, dst, KIO::HideProgressInfo);
        KJobWidgets::setWindow(job, mAuthWindow);
        QObject::connect(job, SIGNAL(result(KJob*)),
                         q, SLOT(slotCopyDone(KJob*)));
        QObject::connect(job, SIGNAL(percent(KJob*,ulong)),
                         q, SLOT(slotPercent(KJob*,ulong)));
    }

    void renameImportedUrl(const QUrl& src)
    {
        QUrl dst = src.resolved(QUrl(".."));
        QString fileName;
        if (mFileNameFormater.get()) {
            KFileItem item(src);
            item.setDelayedMimeTypes(true);
            // Get the document time, but do not cache the result because the
            // 'src' url is temporary: if we import "foo/image.jpg" and
            // "bar/image.jpg", both images will be temporarily saved in the
            // 'src' url.
            QDateTime dateTime = TimeUtils::dateTimeForFileItem(item, TimeUtils::SkipCache);
            fileName = mFileNameFormater->format(src, dateTime);
        } else {
            fileName = src.fileName();
        }
        dst.setPath(dst.path() + fileName);

        FileUtils::RenameResult result = FileUtils::rename(src, dst, mAuthWindow);
        switch (result) {
        case FileUtils::RenamedOK:
            mImportedUrlList << mCurrentUrl;
            break;
        case FileUtils::RenamedUnderNewName:
            mRenamedCount++;
            mImportedUrlList << mCurrentUrl;
            break;
        case FileUtils::Skipped:
            mSkippedUrlList << mCurrentUrl;
            break;
        case FileUtils::RenameFailed:
            qWarning() << "Rename failed for" << mCurrentUrl;
        }
        q->advance();
        importNext();
    }
};

Importer::Importer(QWidget* parent)
: QObject(parent)
, d(new ImporterPrivate)
{
    d->q = this;
    d->mAuthWindow = parent;
}

Importer::~Importer()
{
    delete d;
}

void Importer::setAutoRenameFormat(const QString& format)
{
    if (format.isEmpty()) {
        d->mFileNameFormater.reset(0);
    } else {
        d->mFileNameFormater.reset(new FileNameFormater(format));
    }
}

void Importer::start(const QList<QUrl>& list, const QUrl& destination)
{
    d->mUrlList = list;
    d->mImportedUrlList.clear();
    d->mSkippedUrlList.clear();
    d->mRenamedCount = 0;
    d->mProgress = 0;
    d->mJobProgress = 0;

    emitProgressChanged();
    maximumChanged(d->mUrlList.count() * 100);

    if (!d->createImportDir(destination)) {
        qWarning() << "Could not create import dir";
        return;
    }
    d->importNext();
}

void Importer::slotCopyDone(KJob* _job)
{
    KIO::CopyJob* job = static_cast<KIO::CopyJob*>(_job);
    QUrl url = job->destUrl();
    if (job->error()) {
        qWarning() << "FIXME: What do we do with failed urls?";
        advance();
        d->importNext();
        return;
    }

    d->renameImportedUrl(url);
}

void Importer::finalizeImport()
{
    KIO::Job* job = KIO::del(d->mTempImportDirUrl, KIO::HideProgressInfo);
    KJobWidgets::setWindow(job, d->mAuthWindow);
    importFinished();
}

void Importer::advance()
{
    ++d->mProgress;
    d->mJobProgress = 0;
    emitProgressChanged();
}

void Importer::slotPercent(KJob*, unsigned long percent)
{
    d->mJobProgress = percent;
    emitProgressChanged();
}

void Importer::emitProgressChanged()
{
    progressChanged(d->mProgress * 100 + d->mJobProgress);
}

QList<QUrl> Importer::importedUrlList() const
{
    return d->mImportedUrlList;
}

QList<QUrl> Importer::skippedUrlList() const
{
    return d->mSkippedUrlList;
}

int Importer::renamedCount() const
{
    return d->mRenamedCount;
}

} // namespace
