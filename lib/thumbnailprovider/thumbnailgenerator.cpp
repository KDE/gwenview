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
#include <thumbnailgenerator.moc>

// Local
#include "imageutils.h"
#include "jpegcontent.h"

// KDE
#include <KDebug>

// Qt
#include <QImageReader>
#include <QMatrix>

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

    QImageReader reader(pixPath);
    if (!reader.canRead()) {
        reader.setDecideFormatFromContent(true);
        // Set filename again, otherwise QImageReader won't restart from scratch
        reader.setFileName(pixPath);
    }
    // If it's a Jpeg, try to load an embedded thumbnail, if available
    if (reader.format() == "jpeg") {
        JpegContent content;
        content.load(pixPath);
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
        return false;
    }
    if (!originalSize.isValid()) {
        originalSize = originalImage.size();
    }
    mOriginalWidth = originalSize.width();
    mOriginalHeight = originalSize.height();

    if (qMax(mOriginalWidth, mOriginalHeight) <= pixelSize) {
        mImage = originalImage;
        mNeedCaching = format != "png";
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

//------------------------------------------------------------------------
//
// ThumbnailGenerator
//
//------------------------------------------------------------------------
ThumbnailGenerator::ThumbnailGenerator()
: mCancel(false)
{}

void ThumbnailGenerator::load(
    const QString& originalUri, time_t originalTime, KIO::filesize_t originalSize, const QString& originalMimeType,
    const QString& pixPath,
    const QString& thumbnailPath,
    ThumbnailGroup::Enum group)
{
    QMutexLocker lock(&mMutex);
    Q_ASSERT(mPixPath.isNull());

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

QString ThumbnailGenerator::originalUri() const
{
    return mOriginalUri;
}

time_t ThumbnailGenerator::originalTime() const
{
    return mOriginalTime;
}

KIO::filesize_t ThumbnailGenerator::originalSize() const
{
    return mOriginalSize;
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
                kWarning() << "Could not generate thumbnail for file" << mOriginalUri;
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
    mImage.setText("Thumb::URI"          , 0, mOriginalUri);
    mImage.setText("Thumb::MTime"        , 0, QString::number(mOriginalTime));
    mImage.setText("Thumb::Size"         , 0, QString::number(mOriginalSize));
    mImage.setText("Thumb::Mimetype"     , 0, mOriginalMimeType);
    mImage.setText("Thumb::Image::Width" , 0, QString::number(mOriginalWidth));
    mImage.setText("Thumb::Image::Height", 0, QString::number(mOriginalHeight));
    mImage.setText("Software"            , 0, "Gwenview");

    emit thumbnailReadyToBeCached(mThumbnailPath, mImage);
}

} // namespace
