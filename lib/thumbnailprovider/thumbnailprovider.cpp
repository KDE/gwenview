// vim: set tabstop=4 shiftwidth=4 expandtab:
/*  Gwenview - A simple image viewer for KDE
    Copyright 2000-2007 Aurélien Gâteau <agateau@kde.org>
    This class is based on the ImagePreviewJob class from Konqueror.
*/
/*  This file is part of the KDE project
    Copyright (C) 2000 David Faure <faure@kde.org>
                  2000 Carsten Pfeiffer <pfeiffer@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include "thumbnailprovider.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Qt
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QCryptographicHash>
#include <QDebug>
#include <QTemporaryFile>
#include <QApplication>
#include <QStandardPaths>

// KDE
#include <KIO/JobUiDelegate>
#include <KIO/PreviewJob>
#include <KJobWidgets>

// Local
#include "mimetypeutils.h"
#include "thumbnailwriter.h"
#include "thumbnailgenerator.h"
#include "urlutils.h"

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qDebug() << x
#else
#define LOG(x) ;
#endif

Q_GLOBAL_STATIC(ThumbnailWriter, sThumbnailWriter)

static QString generateOriginalUri(const QUrl &url_)
{
    QUrl url = url_;
    return url.adjusted(QUrl::RemovePassword).url();
}

static QString generateThumbnailPath(const QString& uri, ThumbnailGroup::Enum group)
{
    QString baseDir = ThumbnailProvider::thumbnailBaseDir(group);
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(QFile::encodeName(uri));
    return baseDir + QFile::encodeName(md5.result().toHex()) + ".png";
}

//------------------------------------------------------------------------
//
// ThumbnailProvider static methods
//
//------------------------------------------------------------------------
static QString sThumbnailBaseDir;
QString ThumbnailProvider::thumbnailBaseDir()
{
    if (sThumbnailBaseDir.isEmpty()) {
        const QByteArray customDir = qgetenv("GV_THUMBNAIL_DIR");
        if (customDir.isEmpty()) {
            sThumbnailBaseDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/thumbnails/");
        } else {
            sThumbnailBaseDir = QFile::decodeName(customDir) + '/';
        }
    }
    return sThumbnailBaseDir;
}

void ThumbnailProvider::setThumbnailBaseDir(const QString& dir)
{
    sThumbnailBaseDir = dir;
}

QString ThumbnailProvider::thumbnailBaseDir(ThumbnailGroup::Enum group)
{
    QString dir = thumbnailBaseDir();
    switch (group) {
    case ThumbnailGroup::Normal:
        dir += "normal/";
        break;
    case ThumbnailGroup::Large:
        dir += "large/";
        break;
    }
    return dir;
}

void ThumbnailProvider::deleteImageThumbnail(const QUrl &url)
{
    QString uri = generateOriginalUri(url);
    QFile::remove(generateThumbnailPath(uri, ThumbnailGroup::Normal));
    QFile::remove(generateThumbnailPath(uri, ThumbnailGroup::Large));
}

static void moveThumbnailHelper(const QString& oldUri, const QString& newUri, ThumbnailGroup::Enum group)
{
    QString oldPath = generateThumbnailPath(oldUri, group);
    QString newPath = generateThumbnailPath(newUri, group);
    QImage thumb;
    if (!thumb.load(oldPath)) {
        return;
    }
    thumb.setText("Thumb::URI", newUri);
    thumb.save(newPath, "png");
    QFile::remove(QFile::encodeName(oldPath));
}

void ThumbnailProvider::moveThumbnail(const QUrl &oldUrl, const QUrl& newUrl)
{
    QString oldUri = generateOriginalUri(oldUrl);
    QString newUri = generateOriginalUri(newUrl);
    moveThumbnailHelper(oldUri, newUri, ThumbnailGroup::Normal);
    moveThumbnailHelper(oldUri, newUri, ThumbnailGroup::Large);
}

//------------------------------------------------------------------------
//
// ThumbnailProvider implementation
//
//------------------------------------------------------------------------
ThumbnailProvider::ThumbnailProvider()
: KIO::Job()
, mState(STATE_NEXTTHUMB)
, mOriginalTime(0)
{
    LOG(this);

    // Make sure we have a place to store our thumbnails
    QString thumbnailDirNormal = ThumbnailProvider::thumbnailBaseDir(ThumbnailGroup::Normal);
    QString thumbnailDirLarge = ThumbnailProvider::thumbnailBaseDir(ThumbnailGroup::Large);
    QDir().mkpath(thumbnailDirNormal);
    QDir().mkpath(thumbnailDirLarge);
    QFile::setPermissions(thumbnailDirNormal, QFileDevice::WriteOwner | QFileDevice::ReadOwner | QFileDevice::ExeOwner);
    QFile::setPermissions(thumbnailDirLarge, QFileDevice::WriteOwner | QFileDevice::ReadOwner | QFileDevice::ExeOwner);

    // Look for images and store the items in our todo list
    mCurrentItem = KFileItem();
    mThumbnailGroup = ThumbnailGroup::Large;
    createNewThumbnailGenerator();
}

ThumbnailProvider::~ThumbnailProvider()
{
    LOG(this);
    abortSubjob();
    mThumbnailGenerator->cancel();
    disconnect(mThumbnailGenerator, 0, this, 0);
    disconnect(mThumbnailGenerator, 0, sThumbnailWriter, 0);
    connect(mThumbnailGenerator, SIGNAL(finished()), mThumbnailGenerator, SLOT(deleteLater()));
    if (mPreviousThumbnailGenerator) {
        disconnect(mPreviousThumbnailGenerator, 0, sThumbnailWriter, 0);
    }
    sThumbnailWriter->wait();
}

void ThumbnailProvider::stop()
{
    // Clear mItems and create a new ThumbnailGenerator if mThumbnailGenerator is running,
    // but also make sure that at most two ThumbnailGenerators are running.
    // startCreatingThumbnail() will take care that these two threads won't work on the same item.
    mItems.clear();
    abortSubjob();
    if (mThumbnailGenerator->isRunning() && !mPreviousThumbnailGenerator) {
        mPreviousThumbnailGenerator = mThumbnailGenerator;
        mPreviousThumbnailGenerator->cancel();
        disconnect(mPreviousThumbnailGenerator, 0, this, 0);
        connect(mPreviousThumbnailGenerator, SIGNAL(finished()), mPreviousThumbnailGenerator, SLOT(deleteLater()));
        createNewThumbnailGenerator();
        mCurrentItem = KFileItem();
    }
}

const KFileItemList& ThumbnailProvider::pendingItems() const
{
    return mItems;
}

void ThumbnailProvider::setThumbnailGroup(ThumbnailGroup::Enum group)
{
    mThumbnailGroup = group;
}

void ThumbnailProvider::appendItems(const KFileItemList& items)
{
    if (!mItems.isEmpty()) {
        QSet<QString> itemSet;
        Q_FOREACH(const KFileItem & item, mItems) {
            itemSet.insert(item.url().url());
        }

        Q_FOREACH(const KFileItem & item, items) {
            if (!itemSet.contains(item.url().url())) {
                mItems.append(item);
            }
        }
    } else {
        mItems = items;
    }

    if (mCurrentItem.isNull()) {
        determineNextIcon();
    }
}

void ThumbnailProvider::removeItems(const KFileItemList& itemList)
{
    if (mItems.isEmpty()) {
        return;
    }
    Q_FOREACH(const KFileItem & item, itemList) {
        // If we are removing the next item, update to be the item after or the
        // first if we removed the last item
        mItems.removeAll(item);

        if (item == mCurrentItem) {
            abortSubjob();
        }
    }

    // No more current item, carry on to the next remaining item
    if (mCurrentItem.isNull()) {
        determineNextIcon();
    }
}

void ThumbnailProvider::removePendingItems()
{
    mItems.clear();
}

bool ThumbnailProvider::isRunning() const
{
    return !mCurrentItem.isNull();
}

//-Internal--------------------------------------------------------------
void ThumbnailProvider::createNewThumbnailGenerator()
{
    mThumbnailGenerator = new ThumbnailGenerator;
    connect(mThumbnailGenerator, SIGNAL(done(QImage,QSize)),
            SLOT(thumbnailReady(QImage,QSize)),
            Qt::QueuedConnection);

    connect(mThumbnailGenerator, SIGNAL(thumbnailReadyToBeCached(QString,QImage)),
            sThumbnailWriter, SLOT(queueThumbnail(QString,QImage)),
            Qt::QueuedConnection);
}

void ThumbnailProvider::abortSubjob()
{
    if (hasSubjobs()) {
        LOG("Killing subjob");
        KJob* job = subjobs().first();
        job->kill();
        removeSubjob(job);
        mCurrentItem = KFileItem();
    }
}

void ThumbnailProvider::determineNextIcon()
{
    LOG(this);
    mState = STATE_NEXTTHUMB;

    // No more items ?
    if (mItems.isEmpty()) {
        LOG("No more items. Nothing to do");
        mCurrentItem = KFileItem();
        finished();
        return;
    }

    mCurrentItem = mItems.takeFirst();
    LOG("mCurrentItem.url=" << mCurrentItem.url());

    // First, stat the orig file
    mState = STATE_STATORIG;
    mCurrentUrl = mCurrentItem.url().adjusted(QUrl::NormalizePathSegments);
    mOriginalFileSize = mCurrentItem.size();

    // Do direct stat instead of using KIO if the file is local (faster)
    if (UrlUtils::urlIsFastLocalFile(mCurrentUrl)) {
        QFileInfo fileInfo(mCurrentUrl.toLocalFile());
        mOriginalTime = fileInfo.lastModified().toTime_t();
        QMetaObject::invokeMethod(this, "checkThumbnail", Qt::QueuedConnection);
    } else {
        KIO::Job* job = KIO::stat(mCurrentUrl, KIO::HideProgressInfo);
        KJobWidgets::setWindow(job, qApp->activeWindow());
        LOG("KIO::stat orig" << mCurrentUrl.url());
        addSubjob(job);
    }
    LOG("/determineNextIcon" << this);
}

void ThumbnailProvider::slotResult(KJob * job)
{
    LOG(mState);
    removeSubjob(job);
    Q_ASSERT(subjobs().isEmpty()); // We should have only one job at a time

    switch (mState) {
    case STATE_NEXTTHUMB:
        Q_ASSERT(false);
        determineNextIcon();
        return;

    case STATE_STATORIG: {
        // Could not stat original, drop this one and move on to the next one
        if (job->error()) {
            emitThumbnailLoadingFailed();
            determineNextIcon();
            return;
        }

        // Get modification time of the original file
        KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
        mOriginalTime = entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, -1);
        checkThumbnail();
        return;
    }

    case STATE_DOWNLOADORIG:
        if (job->error()) {
            emitThumbnailLoadingFailed();
            LOG("Delete temp file" << mTempPath);
            QFile::remove(mTempPath);
            mTempPath.clear();
            determineNextIcon();
        } else {
            startCreatingThumbnail(mTempPath);
        }
        return;

    case STATE_PREVIEWJOB:
        determineNextIcon();
        return;
    }
}

void ThumbnailProvider::thumbnailReady(const QImage& _img, const QSize& _size)
{
    QImage img = _img;
    QSize size = _size;
    if (!img.isNull()) {
        emitThumbnailLoaded(img, size);
    } else {
        emitThumbnailLoadingFailed();
    }
    if (!mTempPath.isEmpty()) {
        LOG("Delete temp file" << mTempPath);
        QFile::remove(mTempPath);
        mTempPath.clear();
    }
    determineNextIcon();
}

QImage ThumbnailProvider::loadThumbnailFromCache() const
{
    QImage image = sThumbnailWriter->value(mThumbnailPath);
    if (!image.isNull()) {
        return image;
    }

    image = QImage(mThumbnailPath);
    if (image.isNull() && mThumbnailGroup == ThumbnailGroup::Normal) {
        // If there is a large-sized thumbnail, generate the normal-sized version from it
        QString largeThumbnailPath = generateThumbnailPath(mOriginalUri, ThumbnailGroup::Large);
        QImage largeImage(largeThumbnailPath);
        if (largeImage.isNull()) {
            return image;
        }
        int size = ThumbnailGroup::pixelSize(ThumbnailGroup::Normal);
        image = largeImage.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        Q_FOREACH(const QString& key, largeImage.textKeys()) {
            QString text = largeImage.text(key);
            image.setText(key, text);
        }
        sThumbnailWriter->queueThumbnail(mThumbnailPath, image);
    }

    return image;
}

void ThumbnailProvider::checkThumbnail()
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        determineNextIcon();
        return;
    }

    // If we are in the thumbnail dir, just load the file
    if (mCurrentUrl.isLocalFile()
            && mCurrentUrl.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path().startsWith(thumbnailBaseDir())) {
        QImage image(mCurrentUrl.toLocalFile());
        emitThumbnailLoaded(image, image.size());
        determineNextIcon();
        return;
    }

    mOriginalUri = generateOriginalUri(mCurrentUrl);
    mThumbnailPath = generateThumbnailPath(mOriginalUri, mThumbnailGroup);

    LOG("Stat thumb" << mThumbnailPath);

    QImage thumb = loadThumbnailFromCache();
    KIO::filesize_t fileSize = thumb.text("Thumb::Size").toULongLong();
    if (!thumb.isNull()) {
        if (thumb.text("Thumb::URI") == mOriginalUri &&
                thumb.text("Thumb::MTime").toInt() == mOriginalTime &&
                 (fileSize == 0 || fileSize == mOriginalFileSize)) {
            int width = 0, height = 0;
            QSize size;
            bool ok;

            width = thumb.text("Thumb::Image::Width").toInt(&ok);
            if (ok) height = thumb.text("Thumb::Image::Height").toInt(&ok);
            if (ok) {
                size = QSize(width, height);
            } else {
                qWarning() << "Thumbnail for" << mOriginalUri << "does not contain correct image size information";
                // Don't try to determine the size of a video, it probably won't work and
                // will cause high I/O usage with big files (bug #307007).
                if (MimeTypeUtils::urlKind(mCurrentUrl) == MimeTypeUtils::KIND_VIDEO) {
                    emitThumbnailLoaded(thumb, QSize());
                    determineNextIcon();
                    return;
                }
            }
            emitThumbnailLoaded(thumb, size);
            determineNextIcon();
            return;
        }
    }

    // Thumbnail not found or not valid
    if (MimeTypeUtils::fileItemKind(mCurrentItem) == MimeTypeUtils::KIND_RASTER_IMAGE) {
        if (mCurrentUrl.isLocalFile()) {
            // Original is a local file, create the thumbnail
            startCreatingThumbnail(mCurrentUrl.toLocalFile());
        } else {
            // Original is remote, download it
            mState = STATE_DOWNLOADORIG;

            QTemporaryFile tempFile;
            tempFile.setAutoRemove(false);
            if (!tempFile.open()) {
                qWarning() << "Couldn't create temp file to download " << mCurrentUrl.toDisplayString();
                emitThumbnailLoadingFailed();
                determineNextIcon();
                return;
            }
            mTempPath = tempFile.fileName();

            QUrl url = QUrl::fromLocalFile(mTempPath);
            KIO::Job* job = KIO::file_copy(mCurrentUrl, url, -1, KIO::Overwrite | KIO::HideProgressInfo);
            KJobWidgets::setWindow(job, qApp->activeWindow());
            LOG("Download remote file" << mCurrentUrl.toDisplayString() << "to" << url.toDisplayString());
            addSubjob(job);
        }
    } else {
        // Not a raster image, use a KPreviewJob
        LOG("Starting a KPreviewJob for" << mCurrentItem.url());
        mState = STATE_PREVIEWJOB;
        KFileItemList list;
        list.append(mCurrentItem);
        const int pixelSize = ThumbnailGroup::pixelSize(mThumbnailGroup);
        if (mPreviewPlugins.isEmpty()) {
            mPreviewPlugins = KIO::PreviewJob::availablePlugins();
        }
        KIO::Job* job = KIO::filePreview(list, QSize(pixelSize, pixelSize), &mPreviewPlugins);
        //KJobWidgets::setWindow(job, qApp->activeWindow());
        connect(job, SIGNAL(gotPreview(KFileItem,QPixmap)),
                this, SLOT(slotGotPreview(KFileItem,QPixmap)));
        connect(job, SIGNAL(failed(KFileItem)),
                this, SLOT(emitThumbnailLoadingFailed()));
        addSubjob(job);
    }
}

void ThumbnailProvider::startCreatingThumbnail(const QString& pixPath)
{
    LOG("Creating thumbnail from" << pixPath);
    // If mPreviousThumbnailGenerator is already working on our current item
    // its thumbnail will be passed to sThumbnailWriter when ready. So we
    // connect mPreviousThumbnailGenerator's signal "finished" to determineNextIcon
    // which will load the thumbnail from sThumbnailWriter or from disk
    // (because we re-add mCurrentItem to mItems).
    if (mPreviousThumbnailGenerator && mPreviousThumbnailGenerator->isRunning() &&
        mOriginalUri == mPreviousThumbnailGenerator->originalUri() &&
        mOriginalTime == mPreviousThumbnailGenerator->originalTime() &&
        mOriginalFileSize == mPreviousThumbnailGenerator->originalFileSize() &&
        mCurrentItem.mimetype() == mPreviousThumbnailGenerator->originalMimeType()) {
            connect(mPreviousThumbnailGenerator, SIGNAL(finished()), SLOT(determineNextIcon()));
            mItems.prepend(mCurrentItem);
            return;
    }
    mThumbnailGenerator->load(mOriginalUri, mOriginalTime, mOriginalFileSize,
                          mCurrentItem.mimetype(), pixPath, mThumbnailPath, mThumbnailGroup);
}

void ThumbnailProvider::slotGotPreview(const KFileItem& item, const QPixmap& pixmap)
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        return;
    }
    LOG(mCurrentItem.url());
    QSize size;
    emit thumbnailLoaded(item, pixmap, size, mOriginalFileSize);
}

void ThumbnailProvider::emitThumbnailLoaded(const QImage& img, const QSize& size)
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        return;
    }
    LOG(mCurrentItem.url());
    QPixmap thumb = QPixmap::fromImage(img);
    emit thumbnailLoaded(mCurrentItem, thumb, size, mOriginalFileSize);
}

void ThumbnailProvider::emitThumbnailLoadingFailed()
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        return;
    }
    LOG(mCurrentItem.url());
    emit thumbnailLoadingFailed(mCurrentItem);
}

bool ThumbnailProvider::isThumbnailWriterEmpty()
{
    return sThumbnailWriter->isEmpty();
}

} // namespace
