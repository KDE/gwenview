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
#include "fileutils.h"

// Qt
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QUrl>

// KF
#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/StatJob>
#include <KIO/StoredTransferJob>
#include <KJobWidgets>

// Local
#include "gwenview_importer_debug.h"

namespace Gwenview
{
namespace FileUtils
{
bool contentsAreIdentical(const QUrl &url1, const QUrl &url2, QWidget *authWindow)
{
    KIO::StatJob *statJob = nullptr;
    KIO::StoredTransferJob *fileJob = nullptr;
    QScopedPointer<QIODevice> file1, file2;
    QByteArray file1Bytes, file2Bytes;

    if (url1.isLocalFile()) {
        statJob = KIO::mostLocalUrl(url1);
        KJobWidgets::setWindow(statJob, authWindow);
        if (!statJob->exec()) {
            qCWarning(GWENVIEW_IMPORTER_LOG) << "Unable to stat" << url1;
            return false;
        }
        file1.reset(new QFile(statJob->mostLocalUrl().toLocalFile()));
    } else { // file1 is remote
        fileJob = KIO::storedGet(url1, KIO::NoReload, KIO::HideProgressInfo);
        KJobWidgets::setWindow(fileJob, authWindow);
        if (!fileJob->exec()) {
            qCWarning(GWENVIEW_IMPORTER_LOG) << "Can't read" << url1;
            return false;
        }
        file1Bytes = QByteArray(fileJob->data());
        file1.reset(new QBuffer(&file1Bytes));
    }
    if (!file1->open(QIODevice::ReadOnly)) {
        // Can't read url1, assume it's different from url2
        qCWarning(GWENVIEW_IMPORTER_LOG) << "Can't read" << url1;
        return false;
    }

    if (url2.isLocalFile()) {
        statJob = KIO::mostLocalUrl(url2);
        KJobWidgets::setWindow(statJob, authWindow);
        if (!statJob->exec()) {
            qCWarning(GWENVIEW_IMPORTER_LOG) << "Unable to stat" << url2;
            return false;
        }
        file2.reset(new QFile(statJob->mostLocalUrl().toLocalFile()));
    } else { // file2 is remote
        fileJob = KIO::storedGet(url2, KIO::NoReload, KIO::HideProgressInfo);
        KJobWidgets::setWindow(fileJob, authWindow);
        if (!fileJob->exec()) {
            qCWarning(GWENVIEW_IMPORTER_LOG) << "Can't read" << url2;
            return false;
        }
        file2Bytes = QByteArray(fileJob->data());
        file2.reset(new QBuffer(&file2Bytes));
    }
    if (!file2->open(QIODevice::ReadOnly)) {
        // Can't read url2, assume it's different from url1
        qCWarning(GWENVIEW_IMPORTER_LOG) << "Can't read" << url2;
        return false;
    }

    const int CHUNK_SIZE = 4096;
    while (!file1->atEnd() && !file2->atEnd()) {
        QByteArray url1Array = file1->read(CHUNK_SIZE);
        QByteArray url2Array = file2->read(CHUNK_SIZE);

        if (url1Array != url2Array) {
            return false;
        }
    }
    if (file1->atEnd() && file2->atEnd()) {
        return true;
    } else {
        qCWarning(GWENVIEW_IMPORTER_LOG) << "One file ended before the other";
        return false;
    }
}

RenameResult rename(const QUrl &src, const QUrl &dst_, QWidget *authWindow)
{
    QUrl dst = dst_;
    RenameResult result = RenamedOK;
    int count = 1;

    QFileInfo fileInfo(dst.fileName());
    QString prefix = fileInfo.completeBaseName() + QLatin1Char('_');
    QString suffix = '.' + fileInfo.suffix();

    // Get src size
    KIO::StatJob *sourceStat = KIO::stat(src);
    KJobWidgets::setWindow(sourceStat, authWindow);
    if (!sourceStat->exec()) {
        return RenameFailed;
    }
    KFileItem item(sourceStat->statResult(), src, true /* delayedMimeTypes */);
    KIO::filesize_t srcSize = item.size();

    // Find unique name
    KIO::StatJob *statJob = KIO::stat(dst);
    KJobWidgets::setWindow(statJob, authWindow);
    while (statJob->exec()) {
        // File exists. If it's not the same, try to create a new name
        item = KFileItem(statJob->statResult(), dst, true /* delayedMimeTypes */);
        KIO::filesize_t dstSize = item.size();

        if (srcSize == dstSize && contentsAreIdentical(src, dst, authWindow)) {
            // Already imported, skip it
            KIO::Job *job = KIO::file_delete(src, KIO::HideProgressInfo);
            KJobWidgets::setWindow(job, authWindow);

            return Skipped;
        }
        result = RenamedUnderNewName;

        dst.setPath(dst.adjusted(QUrl::RemoveFilename).path() + prefix + QString::number(count) + suffix);
        statJob = KIO::stat(dst);
        KJobWidgets::setWindow(statJob, authWindow);

        ++count;
    }

    // Rename
    KIO::Job *job = KIO::moveAs(src, dst, KIO::HideProgressInfo);
    KJobWidgets::setWindow(job, authWindow);
    if (!job->exec()) {
        result = RenameFailed;
    }
    return result;
}

} // namespace
} // namespace
