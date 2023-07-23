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
#include <QTemporaryDir>
#include <QUrl>

// KF
#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/MkpathJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <kio/jobclasses.h>

// stdc++
#include <memory>

// Local
#include "gwenview_importer_debug.h"
#include <QDir>
#include <filenameformater.h>
#include <fileutils.h>
#include <lib/timeutils.h>
#include <lib/urlutils.h>

namespace Gwenview
{
struct ImporterPrivate {
    Importer *q = nullptr;
    QWidget *mAuthWindow = nullptr;
    std::unique_ptr<FileNameFormater> mFileNameFormater;
    QUrl mTempImportDirUrl;
    QTemporaryDir *mTempImportDir = nullptr;
    QUrl mDestinationDirUrl;

    /* @defgroup reset Should be reset in start()
     * @{ */
    QList<QUrl> mUrlList;
    QList<QUrl> mImportedUrlList;
    QList<QUrl> mSkippedUrlList;
    QList<QUrl> mFailedUrlList;
    QList<QUrl> mFailedSubFolderList;
    int mRenamedCount;
    int mProgress;
    int mJobProgress;
    /* @} */

    QUrl mCurrentUrl;

    bool createImportDir(const QUrl &url)
    {
        KIO::Job *job = KIO::mkpath(url, QUrl(), KIO::HideProgressInfo);
        KJobWidgets::setWindow(job, mAuthWindow);
        if (!job->exec()) {
            Q_EMIT q->error(i18n("Could not create destination folder."));
            return false;
        }

        // Check if local and fast url. The check for fast url is needed because
        // otherwise the retrieved date will not be correct: see implementation
        // of TimeUtils::dateTimeForFileItem
        if (UrlUtils::urlIsFastLocalFile(url)) {
            QString tempDirPath = url.toLocalFile() + "/.gwenview_importer-XXXXXX";
            mTempImportDir = new QTemporaryDir(tempDirPath);
        } else {
            mTempImportDir = new QTemporaryDir();
        }

        if (!mTempImportDir->isValid()) {
            Q_EMIT q->error(i18n("Could not create temporary upload folder."));
            return false;
        }

        mTempImportDirUrl = QUrl::fromLocalFile(mTempImportDir->path() + QLatin1Char('/'));
        if (!mTempImportDirUrl.isValid()) {
            Q_EMIT q->error(i18n("Could not create temporary upload folder."));
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
        KIO::Job *job = KIO::copy(mCurrentUrl, dst, KIO::HideProgressInfo | KIO::Overwrite);
        KJobWidgets::setWindow(job, mAuthWindow);
        QObject::connect(job, &KJob::result, q, &Importer::slotCopyDone);
        QObject::connect(job, SIGNAL(percent(KJob *, ulong)), q, SLOT(slotPercent(KJob *, ulong)));
    }

    void renameImportedUrl(const QUrl &src)
    {
        QUrl dst = mDestinationDirUrl;
        QString fileName;
        if (mFileNameFormater.get()) {
            KFileItem item(src);
            item.setDelayedMimeTypes(true);
            // Get the document time, but do not cache the result because the
            // 'src' url is temporary: if we import "foo/image.jpg" and
            // "bar/image.jpg", both images will be temporarily saved in the
            // 'src' url.
            const QDateTime dateTime = TimeUtils::dateTimeForFileItem(item, TimeUtils::SkipCache);
            fileName = mFileNameFormater->format(src, dateTime);
        } else {
            fileName = src.fileName();
        }
        dst.setPath(dst.path() + QLatin1Char('/') + fileName);

        FileUtils::RenameResult result;
        // Create additional subfolders if needed (e.g. when extra slashes in FileNameFormater)
        QUrl subFolder = dst.adjusted(QUrl::RemoveFilename);
        KIO::Job *job = KIO::mkpath(subFolder, QUrl(), KIO::HideProgressInfo);
        KJobWidgets::setWindow(job, mAuthWindow);
        if (!job->exec()) { // if subfolder creation fails
            qCWarning(GWENVIEW_IMPORTER_LOG) << "Could not create subfolder:" << subFolder;
            if (!mFailedSubFolderList.contains(subFolder)) {
                mFailedSubFolderList << subFolder;
            }
            result = FileUtils::RenameFailed;
        } else { // if subfolder creation succeeds
            result = FileUtils::rename(src, dst, mAuthWindow);
        }

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
            mFailedUrlList << mCurrentUrl;
            qCWarning(GWENVIEW_IMPORTER_LOG) << "Rename failed for" << mCurrentUrl;
        }
        q->advance();
        importNext();
    }
};

Importer::Importer(QWidget *parent)
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

void Importer::setAutoRenameFormat(const QString &format)
{
    if (format.isEmpty()) {
        d->mFileNameFormater.reset(nullptr);
    } else {
        d->mFileNameFormater = std::make_unique<FileNameFormater>(format);
    }
}

void Importer::start(const QList<QUrl> &list, const QUrl &destination)
{
    d->mDestinationDirUrl = destination;
    d->mUrlList = list;
    d->mImportedUrlList.clear();
    d->mSkippedUrlList.clear();
    d->mFailedUrlList.clear();
    d->mFailedSubFolderList.clear();
    d->mRenamedCount = 0;
    d->mProgress = 0;
    d->mJobProgress = 0;

    emitProgressChanged();
    Q_EMIT maximumChanged(d->mUrlList.count() * 100);

    if (!d->createImportDir(destination)) {
        qCWarning(GWENVIEW_IMPORTER_LOG) << "Could not create import dir";
        return;
    }
    d->importNext();
}

void Importer::slotCopyDone(KJob *_job)
{
    auto job = static_cast<KIO::CopyJob *>(_job);
    const QUrl url = job->destUrl();
    if (job->error()) {
        // Add document to failed url list and proceed with next one
        d->mFailedUrlList << d->mCurrentUrl;
        advance();
        d->importNext();
        return;
    }

    d->renameImportedUrl(url);
}

void Importer::finalizeImport()
{
    delete d->mTempImportDir;
    Q_EMIT importFinished();
}

void Importer::advance()
{
    ++d->mProgress;
    d->mJobProgress = 0;
    emitProgressChanged();
}

void Importer::slotPercent(KJob *, unsigned long percent)
{
    d->mJobProgress = percent;
    emitProgressChanged();
}

void Importer::emitProgressChanged()
{
    Q_EMIT progressChanged(d->mProgress * 100 + d->mJobProgress);
}

QList<QUrl> Importer::importedUrlList() const
{
    return d->mImportedUrlList;
}

QList<QUrl> Importer::skippedUrlList() const
{
    return d->mSkippedUrlList;
}

QList<QUrl> Importer::failedUrlList() const
{
    return d->mFailedUrlList;
}

QList<QUrl> Importer::failedSubFolderList() const
{
    return d->mFailedSubFolderList;
}

int Importer::renamedCount() const
{
    return d->mRenamedCount;
}

} // namespace

#include "moc_importer.cpp"
