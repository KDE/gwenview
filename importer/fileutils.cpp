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
#include <QFile>
#include <QFileInfo>

// KDE
#include <QDebug>
#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/JobClasses>
#include <KIO/JobUiDelegate>
#include <KIO/NetAccess>
#include <KUrl>

// libc
#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace Gwenview
{
namespace FileUtils
{

bool contentsAreIdentical(const KUrl& url1, const KUrl& url2, QWidget* authWindow)
{
    // FIXME: Support remote urls
    QFile file1(KIO::NetAccess::mostLocalUrl(url1, authWindow).toLocalFile());
    if (!file1.open(QIODevice::ReadOnly)) {
        // Can't read url1, assume it's different from url2
        qWarning() << "Can't read" << url1;
        return false;
    }

    QFile file2(KIO::NetAccess::mostLocalUrl(url2, authWindow).toLocalFile());
    if (!file2.open(QIODevice::ReadOnly)) {
        // Can't read url2, assume it's different from url1
        qWarning() << "Can't read" << url2;
        return false;
    }

    const int CHUNK_SIZE = 4096;
    while (!file1.atEnd() && !file2.atEnd()) {
        QByteArray url1Array = file1.read(CHUNK_SIZE);
        QByteArray url2Array = file2.read(CHUNK_SIZE);

        if (url1Array != url2Array) {
            return false;
        }
    }
    if (file1.atEnd() && file2.atEnd()) {
        return true;
    } else {
        qWarning() << "One file ended before the other";
        return false;
    }
}

RenameResult rename(const KUrl& src, const KUrl& dst_, QWidget* authWindow)
{
    KUrl dst = dst_;
    RenameResult result = RenamedOK;
    int count = 1;

    QFileInfo fileInfo(dst.fileName());
    QString prefix = fileInfo.completeBaseName() + '_';
    QString suffix = '.' + fileInfo.suffix();

    // Get src size
    KIO::UDSEntry udsEntry;
    KIO::NetAccess::stat(src, udsEntry, authWindow);
    KFileItem item(udsEntry, src, true /* delayedMimeTypes */);
    KIO::filesize_t srcSize = item.size();

    // Find unique name
    while (KIO::NetAccess::stat(dst, udsEntry, authWindow)) {
        // File exists. If it's not the same, try to create a new name
        item = KFileItem(udsEntry, dst, true /* delayedMimeTypes */);
        KIO::filesize_t dstSize = item.size();

        if (srcSize == dstSize && contentsAreIdentical(src, dst, authWindow)) {
            // Already imported, skip it
            KIO::Job* job = KIO::file_delete(src, KIO::HideProgressInfo);
            if (job->ui()) {
                job->ui()->setWindow(authWindow);
            }

            return Skipped;
        }
        result = RenamedUnderNewName;

        dst.setFileName(prefix + QString::number(count) + suffix);
        ++count;
    }

    // Rename
    KIO::Job* job = KIO::rename(src, dst, KIO::HideProgressInfo);
    if (!KIO::NetAccess::synchronousRun(job, authWindow)) {
        result = RenameFailed;
    }
    return result;
}

QString createTempDir(const QString& baseDir, const QString& prefix, QString* errorMessage)
{
    Q_ASSERT(errorMessage);

    QByteArray name = QFile::encodeName(baseDir + '/' + prefix + "XXXXXX");

    if (!mkdtemp(name.data())) {
        // Failure
        *errorMessage = QString::fromLocal8Bit(::strerror(errno));
        return QString();
    }
    return QFile::decodeName(name + '/');
}

} // namespace
} // namespace
