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
#include "thumbnailgenerator.h"

// Local
#include "imageutils.h"
#include "jpegcontent.h"
#include "gwenviewconfig.h"
#include "exiv2imageloader.h"

// KDE
#include <QDebug>
#ifdef KDCRAW_FOUND
#include <kdcraw/kdcraw.h>
#endif

// Qt
#include <QImageReader>
#include <QMatrix>
#include <QBuffer>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) //qDebug() << x
#else
#define LOG(x) ;
#endif

const int MIN_PREV_SIZE = 1000;

//------------------------------------------------------------------------
//
// ThumbnailContext
//
//------------------------------------------------------------------------
bool ThumbnailContext::load(const QString &pixPath, int pixelSize)
{
    mImage = QImage();
    mNeedCaching = true;
    Orientation orientation = NORMAL;
    QImage originalImage;
    QSize originalSize;

    QByteArray formatHint = pixPath.section('.', -1).toAscii().toLower();
    QImageReader reader(pixPath);

    JpegContent content;
    QByteArray format;
    QByteArray data;
    QBuffer buffer;
    int previewRatio = 1;

#ifdef KDCRAW_FOUND
    // raw images deserve special treatment
    if (KDcrawIface::KDcraw::rawFilesList().contains(QString(formatHint))) {
        // use KDCraw to extract the preview
        bool ret = KDcrawIface::KDcraw::loadEmbeddedPreview(data, pixPath);

        // We need QImage. Loading JpegContent from QImage - exif lost
        // Loading QImage from JpegContent - unimplemented, would go with loadFromData
        if (!ret || !originalImage.loadFromData(data) || qMin(originalImage.width(), originalImage.height()) < MIN_PREV_SIZE) {
            // if the emebedded preview loading failed or gets just a small image, load
            // half preview instead. That's slower...
            if (!KDcrawIface::KDcraw::loadHalfPreview(data, pixPath)) {
                qWarning() << "unable to get preview for " << pixPath.toUtf8().constData();
                return false;
            }
            previewRatio = 2;
        }

        // And we need JpegContent too because of EXIF (orientation!).
        if (!content.loadFromData(data)) {
            qWarning() << "unable to load preview for " << pixPath.toUtf8().constData();
            return false;
        }

        buffer.setBuffer(&data);
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
        reader.setFormat(formatHint);
        mIsRawImage = true;
    } else {
#else
    {
#endif
        if (!reader.canRead()) {
            reader.setDecideFormatFromContent(true);
            // Set filename again, otherwise QImageReader won't restart from scratch
            reader.setFileName(pixPath);
        }

        if (reader.format() == "jpeg" && GwenviewConfig::applyExifOrientation()) {
            content.load(pixPath);
        }
    }

    // If there's jpeg content (from jpg or raw files), try to load an embedded thumbnail, if available.
    // If applyExifOrientation is not set, don't use the
    // embedded thumbnail since it might be rotated differently
    // than the actual image
    if (!content.rawData().isEmpty() && GwenviewConfig::applyExifOrientation() && !mIsRawImage) {
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
    originalSize = reader.size();
    if (originalSize.isValid() && reader.supportsOption(QImageIOHandler::ScaledSize)) {
        QSizeF scaledSize = originalSize;
        scaledSize.scale(pixelSize, pixelSize, Qt::KeepAspectRatio);
        if (!scaledSize.isEmpty()) {
            reader.setScaledSize(scaledSize.toSize());
        }
    }

    // format() is empty after QImageReader::read() is called
    format = reader.format();
    if (!reader.read(&originalImage)) {
        return false;
    }

    if (!originalSize.isValid()) {
        originalSize = originalImage.size();
    }
    mOriginalWidth = originalSize.width() * previewRatio;
    mOriginalHeight = originalSize.height() * previewRatio;

    if (qMax(mOriginalWidth, mOriginalHeight) <= pixelSize) {
        mImage = originalImage;
        mNeedCaching = format != "png";
    } else {
        mImage = originalImage.scaled(pixelSize, pixelSize, Qt::KeepAspectRatio);
    }

    // Rotate if necessary
    if (orientation != NORMAL && orientation != NOT_AVAILABLE && GwenviewConfig::applyExifOrientation()) {
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

//------------------------------------------------------------------------
//
// ThumbnailGenerator
//
//------------------------------------------------------------------------
ThumbnailGenerator::ThumbnailGenerator()
: mCancel(false)
{}

void ThumbnailGenerator::load(
    const QString& originalUri, time_t originalTime, KIO::filesize_t originalFileSize, const QString& originalMimeType,
    const QString& pixPath,
    const QString& thumbnailPath,
    ThumbnailGroup::Enum group)
{
    QMutexLocker lock(&mMutex);
    Q_ASSERT(mPixPath.isNull());

    mOriginalUri = originalUri;
    mOriginalTime = originalTime;
    mOriginalFileSize = originalFileSize;
    mOriginalMimeType = originalMimeType;
    mPixPath = pixPath;
    mThumbnailPath = thumbnailPath;
    mThumbnailGroup = group;
    if (!isRunning()) start();
    mCond.wakeOne();
}

QString ThumbnailGenerator::originalUri() const
{
    return mOriginalUri;
}

time_t ThumbnailGenerator::originalTime() const
{
    return mOriginalTime;
}

KIO::filesize_t ThumbnailGenerator::originalFileSize() const
{
    return mOriginalFileSize;
}

QString ThumbnailGenerator::originalMimeType() const
{
    return mOriginalMimeType;
}

bool ThumbnailGenerator::testCancel()
{
    QMutexLocker lock(&mMutex);
    return mCancel;
}

void ThumbnailGenerator::cancel()
{
    QMutexLocker lock(&mMutex);
    mCancel = true;
    mCond.wakeOne();
}

void ThumbnailGenerator::run()
{
    LOG("");
    while (!testCancel()) {
        QString pixPath;
        int pixelSize;
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
            pixPath = mPixPath;
            pixelSize = ThumbnailGroup::pixelSize(mThumbnailGroup);
        }

        Q_ASSERT(!pixPath.isNull());
        LOG("Loading" << pixPath);
        ThumbnailContext context;
        bool ok = context.load(pixPath, pixelSize);

        {
            QMutexLocker lock(&mMutex);
            if (ok) {
                mImage = context.mImage;
                mOriginalWidth = context.mOriginalWidth;
                mOriginalHeight = context.mOriginalHeight;
                if (context.mNeedCaching) {
                    cacheThumbnail();
                }
            } else {
                qWarning() << "Could not generate thumbnail for file" << mOriginalUri;
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

void ThumbnailGenerator::cacheThumbnail()
{
    mImage.setText("Thumb::URI"          , mOriginalUri);
    mImage.setText("Thumb::MTime"        , QString::number(mOriginalTime));
    mImage.setText("Thumb::Size"         , QString::number(mOriginalFileSize));
    mImage.setText("Thumb::Mimetype"     , mOriginalMimeType);
    mImage.setText("Thumb::Image::Width" , QString::number(mOriginalWidth));
    mImage.setText("Thumb::Image::Height", QString::number(mOriginalHeight));
    mImage.setText("Software"            , QStringLiteral("Gwenview"));

    emit thumbnailReadyToBeCached(mThumbnailPath, mImage);
}

} // namespace
