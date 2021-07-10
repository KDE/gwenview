// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
#include "thumbnailwriter.h"

// Local
#include "gwenview_lib_debug.h"
#include "gwenviewconfig.h"

// Qt
#include <QImage>
#include <QTemporaryFile>

namespace Gwenview
{
#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) // qCDebug(GWENVIEW_LIB_LOG) << x
#else
#define LOG(x) ;
#endif

static void storeThumbnailToDiskCache(const QString &path, const QImage &image)
{
    if (GwenviewConfig::lowResourceUsageMode()) {
        return;
    }

    LOG(path);
    QTemporaryFile tmp(path + QStringLiteral(".gwenview.tmpXXXXXX.png"));
    if (!tmp.open()) {
        qCWarning(GWENVIEW_LIB_LOG) << "Could not create a temporary file.";
        return;
    }

    if (!image.save(tmp.fileName(), "png")) {
        qCWarning(GWENVIEW_LIB_LOG) << "Could not save thumbnail";
        return;
    }

    QFile::rename(tmp.fileName(), path);
}

void ThumbnailWriter::queueThumbnail(const QString &path, const QImage &image)
{
    if (GwenviewConfig::lowResourceUsageMode()) {
        return;
    }

    LOG(path);
    QMutexLocker locker(&mMutex);
    mCache.insert(path, image);
    start();
}

void ThumbnailWriter::run()
{
    QMutexLocker locker(&mMutex);
    while (!mCache.isEmpty() && !isInterruptionRequested()) {
        Cache::ConstIterator it = mCache.constBegin();
        const QString path = it.key();
        const QImage image = it.value();

        // This part of the thread is the most time consuming but it does not
        // depend on mCache so we can unlock here. This way other thumbnails
        // can be added or queried
        locker.unlock();
        storeThumbnailToDiskCache(path, image);
        locker.relock();

        mCache.remove(path);
    }
}

QImage ThumbnailWriter::value(const QString &path) const
{
    QMutexLocker locker(&mMutex);
    return mCache.value(path);
}

bool ThumbnailWriter::isEmpty() const
{
    QMutexLocker locker(&mMutex);
    return mCache.isEmpty();
}

} // namespace
