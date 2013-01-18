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
#include "thumbnailloadjob.moc"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

// Qt
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QMatrix>
#include <QPixmap>
#include <QTimer>

// KDE
#include <KApplication>
#include <KCodecs>
#include <KDebug>
#include <kde_file.h>
#include <KFileItem>
#include <KIO/JobUiDelegate>
#include <KIO/PreviewJob>
#include <KStandardDirs>
#include <KTemporaryFile>

// Local
#include "mimetypeutils.h"
#include "jpegcontent.h"
#include "imageutils.h"
#include "urlutils.h"

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

static QString generateOriginalUri(const KUrl& url_)
{
    KUrl url = url_;
    // Don't include the password if any
    url.setPass(QString::null); //krazy:exclude=nullstrassign for old broken gcc
    return url.url();
}

static QString generateThumbnailPath(const QString& uri, ThumbnailGroup::Enum group)
{
    KMD5 md5(QFile::encodeName(uri));
    QString baseDir = ThumbnailLoadJob::thumbnailBaseDir(group);
    return baseDir + QString(QFile::encodeName(md5.hexDigest())) + ".png";
}

//------------------------------------------------------------------------
//
// ThumbnailCache
//
//------------------------------------------------------------------------
K_GLOBAL_STATIC(ThumbnailCache, sThumbnailCache)

static void storeThumbnailToDiskCache(const QString& path, const QImage& image)
{
    LOG(path);
    KTemporaryFile tmp;
    tmp.setPrefix(path + ".gwenview.tmp");
    tmp.setSuffix(".png");
    if (!tmp.open()) {
        kWarning() << "Could not create a temporary file.";
        return;
    }

    if (!image.save(tmp.fileName(), "png")) {
        kWarning() << "Could not save thumbnail";
        return;
    }

    KDE_rename(QFile::encodeName(tmp.fileName()), QFile::encodeName(path));
}

void ThumbnailCache::queueThumbnail(const QString& path, const QImage& image)
{
    LOG(path);
    QMutexLocker locker(&mMutex);
    mCache.insert(path, image);
    start();
}

void ThumbnailCache::run()
{
    QMutexLocker locker(&mMutex);
    while (!mCache.isEmpty()) {
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

QImage ThumbnailCache::value(const QString& path) const
{
    QMutexLocker locker(&mMutex);
    return mCache.value(path);
}

bool ThumbnailCache::isEmpty() const
{
    QMutexLocker locker(&mMutex);
    return mCache.isEmpty();
}

//------------------------------------------------------------------------
//
// ThumbnailThread
//
//------------------------------------------------------------------------
ThumbnailThread::ThumbnailThread()
: mCancel(false)
{}

void ThumbnailThread::load(
    const QString& originalUri, time_t originalTime, int originalSize, const QString& originalMimeType,
    const QString& pixPath,
    const QString& thumbnailPath,
    ThumbnailGroup::Enum group)
{
    QMutexLocker lock(&mMutex);
    assert(mPixPath.isNull());

    mOriginalUri = originalUri;
    mOriginalTime = originalTime;
    mOriginalSize = originalSize;
    mOriginalMimeType = originalMimeType;
    mPixPath = pixPath;
    mThumbnailPath = thumbnailPath;
    mThumbnailGroup = group;
    if (!isRunning()) start();
    mCond.wakeOne();
}

bool ThumbnailThread::testCancel()
{
    QMutexLocker lock(&mMutex);
    return mCancel;
}

void ThumbnailThread::cancel()
{
    QMutexLocker lock(&mMutex);
    mCancel = true;
    mCond.wakeOne();
}

void ThumbnailThread::run()
{
    LOG("");
    while (!testCancel()) {
        {
            QMutexLocker lock(&mMutex);
            // empty mPixPath means nothing to do
            LOG("Waiting for mPixPath");
            if (mPixPath.isNull()) {
                LOG("mPixPath.isNull");
                mCond.wait(&mMutex);
            }
        }
        if (testCancel()) {
            return;
        }
        {
            QMutexLocker lock(&mMutex);
            Q_ASSERT(!mPixPath.isNull());
            LOG("Loading" << mPixPath);
            bool needCaching;
            bool ok = loadThumbnail(&needCaching);
            if (ok && needCaching) {
                cacheThumbnail();
            }
            mPixPath.clear(); // done, ready for next
        }
        if (testCancel()) {
            return;
        }
        {
            QSize size(mOriginalWidth, mOriginalHeight);
            LOG("emitting done signal, size=" << size);
            QMutexLocker lock(&mMutex);
            done(mImage, size);
            LOG("Done");
        }
    }
    LOG("Ending thread");
}

bool ThumbnailThread::loadThumbnail(bool* needCaching)
{
    mImage = QImage();
    *needCaching = true;
    int pixelSize = ThumbnailGroup::pixelSize(mThumbnailGroup);
    Orientation orientation = NORMAL;

    QImageReader reader(mPixPath);
    if (!reader.canRead()) {
        reader.setDecideFormatFromContent(true);
        reader.setFileName(mPixPath);
    }
    // If it's a Jpeg, try to load an embedded thumbnail, if available
    if (reader.format() == "jpeg") {
        JpegContent content;
        content.load(mPixPath);
        QImage thumbnail = content.thumbnail();
        orientation = content.orientation();

        if (qMax(thumbnail.width(), thumbnail.height()) >= pixelSize) {
            mImage = thumbnail;
            if (orientation != NORMAL && orientation != NOT_AVAILABLE) {
                QMatrix matrix = ImageUtils::transformMatrix(orientation);
                mImage = mImage.transformed(matrix);
            }
            mOriginalWidth = content.size().width();
            mOriginalHeight = content.size().height();
            return true;
        }
    }

    // Generate thumbnail from full image
    QSize originalSize = reader.size();
    if (originalSize.isValid() && reader.supportsOption(QImageIOHandler::ScaledSize)) {
        QSizeF scaledSize = originalSize;
        scaledSize.scale(pixelSize, pixelSize, Qt::KeepAspectRatio);
        if (!scaledSize.isEmpty()) {
            reader.setScaledSize(scaledSize.toSize());
        }
    }

    QImage originalImage;
    // format() is empty after QImageReader::read() is called
    QByteArray format = reader.format();
    if (!reader.read(&originalImage)) {
        kWarning() << "Could not generate thumbnail for file" << mOriginalUri;
        return false;
    }
    if (!originalSize.isValid()) {
        originalSize = originalImage.size();
    }
    mOriginalWidth = originalSize.width();
    mOriginalHeight = originalSize.height();

    if (qMax(mOriginalWidth, mOriginalHeight) <= pixelSize) {
        mImage = originalImage;
        *needCaching = format != "png";
    } else {
        mImage = originalImage.scaled(pixelSize, pixelSize, Qt::KeepAspectRatio);
    }

    // Rotate if necessary
    if (orientation != NORMAL && orientation != NOT_AVAILABLE) {
        QMatrix matrix = ImageUtils::transformMatrix(orientation);
        mImage = mImage.transformed(matrix);

        switch (orientation) {
        case TRANSPOSE:
        case ROT_90:
        case TRANSVERSE:
        case ROT_270:
            qSwap(mOriginalWidth, mOriginalHeight);
            break;
        default:
            break;
        }
    }
    return true;
}

void ThumbnailThread::cacheThumbnail()
{
    mImage.setText("Thumb::URI"          , 0, mOriginalUri);
    mImage.setText("Thumb::MTime"        , 0, QString::number(mOriginalTime));
    mImage.setText("Thumb::Size"         , 0, QString::number(mOriginalSize));
    mImage.setText("Thumb::Mimetype"     , 0, mOriginalMimeType);
    mImage.setText("Thumb::Image::Width" , 0, QString::number(mOriginalWidth));
    mImage.setText("Thumb::Image::Height", 0, QString::number(mOriginalHeight));
    mImage.setText("Software"            , 0, "Gwenview");

    emit thumbnailReadyToBeCached(mThumbnailPath, mImage);
}

//------------------------------------------------------------------------
//
// ThumbnailLoadJob static methods
//
//------------------------------------------------------------------------
static QString sThumbnailBaseDir;
QString ThumbnailLoadJob::thumbnailBaseDir()
{
    if (sThumbnailBaseDir.isEmpty()) {
        const QByteArray customDir = qgetenv("GV_THUMBNAIL_DIR");
        if (customDir.isEmpty()) {
            sThumbnailBaseDir = QDir::homePath() + "/.thumbnails/";
        } else {
            sThumbnailBaseDir = QString::fromLocal8Bit(customDir.constData()) + '/';
        }
    }
    return sThumbnailBaseDir;
}

void ThumbnailLoadJob::setThumbnailBaseDir(const QString& dir)
{
    sThumbnailBaseDir = dir;
}

QString ThumbnailLoadJob::thumbnailBaseDir(ThumbnailGroup::Enum group)
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

void ThumbnailLoadJob::deleteImageThumbnail(const KUrl& url)
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
    thumb.setText("Thumb::URI", 0, newUri);
    thumb.save(newPath, "png");
    QFile::remove(QFile::encodeName(oldPath));
}

void ThumbnailLoadJob::moveThumbnail(const KUrl& oldUrl, const KUrl& newUrl)
{
    QString oldUri = generateOriginalUri(oldUrl);
    QString newUri = generateOriginalUri(newUrl);
    moveThumbnailHelper(oldUri, newUri, ThumbnailGroup::Normal);
    moveThumbnailHelper(oldUri, newUri, ThumbnailGroup::Large);
}

//------------------------------------------------------------------------
//
// ThumbnailLoadJob implementation
//
//------------------------------------------------------------------------
ThumbnailLoadJob::ThumbnailLoadJob(const KFileItemList& items, ThumbnailGroup::Enum group)
: KIO::Job()
, mState(STATE_NEXTTHUMB)
, mThumbnailGroup(group)
{
    LOG(this);

    // Make sure we have a place to store our thumbnails
    QString thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(mThumbnailGroup);
    KStandardDirs::makeDir(thumbnailDir, 0700);

    // Look for images and store the items in our todo list
    Q_ASSERT(!items.empty());
    mItems = items;
    mCurrentItem = KFileItem();

    connect(&mThumbnailThread, SIGNAL(done(QImage,QSize)),
            SLOT(thumbnailReady(QImage,QSize)),
            Qt::QueuedConnection);

    connect(&mThumbnailThread, SIGNAL(thumbnailReadyToBeCached(QString,QImage)),
            sThumbnailCache, SLOT(queueThumbnail(QString,QImage)),
            Qt::QueuedConnection);
}

ThumbnailLoadJob::~ThumbnailLoadJob()
{
    LOG(this);
    if (hasSubjobs()) {
        LOG("Killing subjob");
        KJob* job = subjobs().first();
        job->kill();
        removeSubjob(job);
    }
    mThumbnailThread.cancel();
    mThumbnailThread.wait();
    sThumbnailCache->wait();
}

void ThumbnailLoadJob::start()
{
    if (mItems.isEmpty()) {
        LOG("Nothing to do");
        emitResult();
        return;
    }

    determineNextIcon();
}

const KFileItemList& ThumbnailLoadJob::pendingItems() const
{
    return mItems;
}

void ThumbnailLoadJob::setThumbnailGroup(ThumbnailGroup::Enum group)
{
    mThumbnailGroup = group;
}

//-Internal--------------------------------------------------------------
void ThumbnailLoadJob::appendItem(const KFileItem& item)
{
    if (!mItems.contains(item)) {
        mItems.append(item);
    }
}

void ThumbnailLoadJob::removeItems(const KFileItemList& itemList)
{
    Q_FOREACH(const KFileItem & item, itemList) {
        // If we are removing the next item, update to be the item after or the
        // first if we removed the last item
        mItems.removeAll(item);

        if (item == mCurrentItem) {
            // Abort current item
            mCurrentItem = KFileItem();
            if (hasSubjobs()) {
                KJob* job = subjobs().first();
                job->kill();
                removeSubjob(job);
            }
        }
    }

    // No more current item, carry on to the next remaining item
    if (mCurrentItem.isNull()) {
        determineNextIcon();
    }
}

void ThumbnailLoadJob::determineNextIcon()
{
    LOG(this);
    mState = STATE_NEXTTHUMB;

    // No more items ?
    if (mItems.isEmpty()) {
        // Done
        LOG("emitting result");
        emitResult();
        return;
    }

    mCurrentItem = mItems.takeFirst();
    LOG("mCurrentItem.url=" << mCurrentItem.url());

    // First, stat the orig file
    mState = STATE_STATORIG;
    mCurrentUrl = mCurrentItem.url();
    mCurrentUrl.cleanPath();

    // Do direct stat instead of using KIO if the file is local (faster)
    bool directStatOk = false;
    if (UrlUtils::urlIsFastLocalFile(mCurrentUrl)) {
        KDE_struct_stat buff;
        if (KDE::stat(mCurrentUrl.toLocalFile(), &buff) == 0)  {
            directStatOk = true;
            mOriginalTime = buff.st_mtime;
            QMetaObject::invokeMethod(this, "checkThumbnail", Qt::QueuedConnection);
        }
    }
    if (!directStatOk) {
        KIO::Job* job = KIO::stat(mCurrentUrl, KIO::HideProgressInfo);
        job->ui()->setWindow(KApplication::kApplication()->activeWindow());
        LOG("KIO::stat orig" << mCurrentUrl.url());
        addSubjob(job);
    }
    LOG("/determineNextIcon" << this);
}

void ThumbnailLoadJob::slotResult(KJob * job)
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

void ThumbnailLoadJob::thumbnailReady(const QImage& _img, const QSize& _size)
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

QImage ThumbnailLoadJob::loadThumbnailFromCache() const
{
    QImage image = sThumbnailCache->value(mThumbnailPath);
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
        sThumbnailCache->queueThumbnail(mThumbnailPath, image);
    }

    return image;
}

void ThumbnailLoadJob::checkThumbnail()
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        determineNextIcon();
        return;
    }

    // If we are in the thumbnail dir, just load the file
    if (mCurrentUrl.isLocalFile()
            && mCurrentUrl.directory().startsWith(thumbnailBaseDir())) {
        QImage image(mCurrentUrl.toLocalFile());
        emitThumbnailLoaded(image, image.size());
        determineNextIcon();
        return;
    }
    QSize imagesize;

    mOriginalUri = generateOriginalUri(mCurrentUrl);
    mThumbnailPath = generateThumbnailPath(mOriginalUri, mThumbnailGroup);

    LOG("Stat thumb" << mThumbnailPath);

    QImage thumb = loadThumbnailFromCache();
    if (!thumb.isNull()) {
        if (thumb.text("Thumb::URI", 0) == mOriginalUri &&
                thumb.text("Thumb::MTime", 0).toInt() == mOriginalTime) {
            int width = 0, height = 0;
            QSize size;
            bool ok;

            width = thumb.text("Thumb::Image::Width", 0).toInt(&ok);
            if (ok) height = thumb.text("Thumb::Image::Height", 0).toInt(&ok);
            if (ok) {
                size = QSize(width, height);
            } else {
                kWarning() << "Thumbnail for" << mOriginalUri << "does not contain correct image size information";
                // Don't try to determine the size of a video, it probably won't work and
                // will cause high I/O usage with big files (bug #307007).
                if (MimeTypeUtils::urlKind(mCurrentUrl) == MimeTypeUtils::KIND_VIDEO) {
                    emitThumbnailLoaded(thumb, QSize());
                    determineNextIcon();
                    return;
                }
                KFileMetaInfo fmi(mCurrentUrl);
                if (fmi.isValid()) {
                    KFileMetaInfoItem item = fmi.item("Dimensions");
                    if (item.isValid()) {
                        size = item.value().toSize();
                    } else {
                        kWarning() << "KFileMetaInfoItem for" << mOriginalUri << "did not get image size information";
                    }
                } else {
                    kWarning() << "Could not get a valid KFileMetaInfo instance for" << mOriginalUri;
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

            KTemporaryFile tempFile;
            tempFile.setAutoRemove(false);
            if (!tempFile.open()) {
                kWarning() << "Couldn't create temp file to download " << mCurrentUrl.prettyUrl();
                emitThumbnailLoadingFailed();
                determineNextIcon();
                return;
            }
            mTempPath = tempFile.fileName();

            KUrl url;
            url.setPath(mTempPath);
            KIO::Job* job = KIO::file_copy(mCurrentUrl, url, -1, KIO::Overwrite | KIO::HideProgressInfo);
            job->ui()->setWindow(KApplication::kApplication()->activeWindow());
            LOG("Download remote file" << mCurrentUrl.prettyUrl() << "to" << url.pathOrUrl());
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
        //job->ui()->setWindow(KApplication::kApplication()->activeWindow());
        connect(job, SIGNAL(gotPreview(KFileItem,QPixmap)),
                this, SLOT(slotGotPreview(KFileItem,QPixmap)));
        connect(job, SIGNAL(failed(KFileItem)),
                this, SLOT(emitThumbnailLoadingFailed()));
        addSubjob(job);
    }
}

void ThumbnailLoadJob::startCreatingThumbnail(const QString& pixPath)
{
    LOG("Creating thumbnail from" << pixPath);
    mThumbnailThread.load(mOriginalUri, mOriginalTime, mCurrentItem.size(),
                          mCurrentItem.mimetype(), pixPath, mThumbnailPath, mThumbnailGroup);
}

void ThumbnailLoadJob::slotGotPreview(const KFileItem& item, const QPixmap& pixmap)
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        return;
    }
    LOG(mCurrentItem.url());
    QSize size;
    emit thumbnailLoaded(item, pixmap, size);
}

void ThumbnailLoadJob::emitThumbnailLoaded(const QImage& img, const QSize& size)
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        return;
    }
    LOG(mCurrentItem.url());
    QPixmap thumb = QPixmap::fromImage(img);
    emit thumbnailLoaded(mCurrentItem, thumb, size);
}

void ThumbnailLoadJob::emitThumbnailLoadingFailed()
{
    if (mCurrentItem.isNull()) {
        // This can happen if current item has been removed by removeItems()
        return;
    }
    LOG(mCurrentItem.url());
    emit thumbnailLoadingFailed(mCurrentItem);
}

bool ThumbnailLoadJob::isPendingThumbnailCacheEmpty()
{
    return sThumbnailCache->isEmpty();
}

} // namespace
