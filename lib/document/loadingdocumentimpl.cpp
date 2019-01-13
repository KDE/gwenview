// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "loadingdocumentimpl.h"

// STL
#include <memory>

// Exiv2
#include <exiv2/exiv2.hpp>

// Qt
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QPointer>
#include <QtConcurrent>
#include <QUrl>
#include <QDebug>

// KDE
#include <KIO/Job>
#include <kio/jobclasses.h>
#include <KLocalizedString>
#include <KProtocolInfo>

#ifdef KDCRAW_FOUND
#include <kdcraw/kdcraw.h>
#endif

// Local
#include "animateddocumentloadedimpl.h"
#include "cms/cmsprofile.h"
#include "document.h"
#include "documentloadedimpl.h"
#include "emptydocumentimpl.h"
#include "exiv2imageloader.h"
#include "gvdebug.h"
#include "imageutils.h"
#include "jpegcontent.h"
#include "jpegdocumentloadedimpl.h"
#include "orientation.h"
#include "svgdocumentloadedimpl.h"
#include "urlutils.h"
#include "videodocumentloadedimpl.h"
#include "gwenviewconfig.h"

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

const int HEADER_SIZE = 256;

struct LoadingDocumentImplPrivate
{
    LoadingDocumentImpl* q;
    QPointer<KIO::TransferJob> mTransferJob;
    QFuture<bool> mMetaInfoFuture;
    QFutureWatcher<bool> mMetaInfoFutureWatcher;
    QFuture<void> mImageDataFuture;
    QFutureWatcher<void> mImageDataFutureWatcher;

    // If != 0, this means we need to load an image at zoom =
    // 1/mImageDataInvertedZoom
    int mImageDataInvertedZoom;

    bool mMetaInfoLoaded;
    bool mAnimated;
    bool mDownSampledImageLoaded;
    QByteArray mFormatHint;
    QByteArray mData;
    QByteArray mFormat;
    QSize mImageSize;
    std::unique_ptr<Exiv2::Image> mExiv2Image;
    std::unique_ptr<JpegContent> mJpegContent;
    QImage mImage;
    Cms::Profile::Ptr mCmsProfile;

    /**
     * Determine kind of document and switch to an implementation if it is not
     * necessary to download more data.
     * @return true if switched to another implementation.
     */
    bool determineKind()
    {
        QString mimeType;
        const QUrl &url = q->document()->url();
        QMimeDatabase db;
        if (KProtocolInfo::determineMimetypeFromExtension(url.scheme())) {
            mimeType = db.mimeTypeForFileNameAndData(url.fileName(), mData).name();
        } else {
            mimeType = db.mimeTypeForData(mData).name();
        }
        MimeTypeUtils::Kind kind = MimeTypeUtils::mimeTypeKind(mimeType);
        LOG("mimeType:" << mimeType);
        LOG("kind:" << kind);
        q->setDocumentKind(kind);

        switch (kind) {
        case MimeTypeUtils::KIND_RASTER_IMAGE:
        case MimeTypeUtils::KIND_SVG_IMAGE:
            return false;

        case MimeTypeUtils::KIND_VIDEO:
            q->switchToImpl(new VideoDocumentLoadedImpl(q->document()));
            return true;

        default:
            q->setDocumentErrorString(
                i18nc("@info", "Gwenview cannot display documents of type %1.", mimeType)
            );
            emit q->loadingFailed();
            q->switchToImpl(new EmptyDocumentImpl(q->document()));
            return true;
        }
    }

    void startLoading()
    {
        Q_ASSERT(!mMetaInfoLoaded);

        switch (q->document()->kind()) {
        case MimeTypeUtils::KIND_RASTER_IMAGE:
            // The hint is used to:
            // - Speed up loadMetaInfo(): QImageReader will try to decode the
            //   image using plugins matching this format first.
            // - Avoid breakage: Because of a bug in Qt TGA image plugin, some
            //   PNG were incorrectly identified as PCX! See:
            //   https://bugs.kde.org/show_bug.cgi?id=289819
            //
            mFormatHint = q->document()->url().fileName()
                .section(QLatin1Char('.'), -1).toLocal8Bit().toLower();
            mMetaInfoFuture = QtConcurrent::run(this, &LoadingDocumentImplPrivate::loadMetaInfo);
            mMetaInfoFutureWatcher.setFuture(mMetaInfoFuture);
            break;

        case MimeTypeUtils::KIND_SVG_IMAGE:
            q->switchToImpl(new SvgDocumentLoadedImpl(q->document(), mData));
            break;

        case MimeTypeUtils::KIND_VIDEO:
            break;

        default:
            qWarning() << "We should not reach this point!";
            break;
        }
    }

    void startImageDataLoading()
    {
        LOG("");
        Q_ASSERT(mMetaInfoLoaded);
        Q_ASSERT(mImageDataInvertedZoom != 0);
        Q_ASSERT(!mImageDataFuture.isRunning());
        mImageDataFuture = QtConcurrent::run(this, &LoadingDocumentImplPrivate::loadImageData);
        mImageDataFutureWatcher.setFuture(mImageDataFuture);
    }

    bool loadMetaInfo()
    {
        LOG("mFormatHint" << mFormatHint);
        QBuffer buffer;
        buffer.setBuffer(&mData);
        buffer.open(QIODevice::ReadOnly);

#ifdef KDCRAW_FOUND
        if (KDcrawIface::KDcraw::rawFilesList().contains(QString::fromLatin1(mFormatHint))) {
            QByteArray previewData;

            // if the image is in format supported by dcraw, fetch its embedded preview
            mJpegContent.reset(new JpegContent());

            // use KDcraw for getting the embedded preview
            // KDcraw functionality cloned locally (temp. solution)
            bool ret = KDcrawIface::KDcraw::loadEmbeddedPreview(previewData, buffer);

            QImage originalImage;
            if (!ret || !originalImage.loadFromData(previewData) || qMin(originalImage.width(), originalImage.height()) < MIN_PREV_SIZE) {
                // if the embedded preview loading failed or gets just a small image, load
                // half preview instead. That's slower but it works even for images containing
                // small (160x120px) or none embedded preview.
                if (!KDcrawIface::KDcraw::loadHalfPreview(previewData, buffer)) {
                    qWarning() << "unable to get half preview for " << q->document()->url().fileName();
                    return false;
                }
            }

            buffer.close();

            // now it's safe to replace mData with the jpeg data
            mData = previewData;

            // need to fill mFormat so gwenview can tell the type when trying to save
            mFormat = mFormatHint;
        } else {
#else
{
#endif
            QImageReader reader(&buffer, mFormatHint);
            mImageSize = reader.size();

            if (!reader.canRead()) {
                qWarning() << "QImageReader::read() using format hint" << mFormatHint << "failed:" << reader.errorString();
                if (buffer.pos() != 0) {
                    qWarning() << "A bad Qt image decoder moved the buffer to" << buffer.pos() << "in a call to canRead()! Rewinding.";
                    buffer.seek(0);
                }
                reader.setFormat(QByteArray());
                // Set buffer again, otherwise QImageReader won't restart from scratch
                reader.setDevice(&buffer);
                if (!reader.canRead()) {
                    qWarning() << "QImageReader::read() without format hint failed:" << reader.errorString();
                    return false;
                }
                qWarning() << "Image format is actually" << reader.format() << "not" << mFormatHint;
            }

            mFormat = reader.format();

            if (mFormat == "jpg") {
                // if mFormatHint was "jpg", then mFormat is "jpg", but the rest of
                // Gwenview code assumes JPEG images have "jpeg" format.
                mFormat = "jpeg";
            }
        }

        LOG("mFormat" << mFormat);
        GV_RETURN_VALUE_IF_FAIL(!mFormat.isEmpty(), false);

        Exiv2ImageLoader loader;
        if (loader.load(mData)) {
            mExiv2Image = loader.popImage();
        }

        if (mFormat == "jpeg" && mExiv2Image.get()) {
            mJpegContent.reset(new JpegContent());
        }

        if (mJpegContent.get()) {
            if (!mJpegContent->loadFromData(mData, mExiv2Image.get()) &&
                !mJpegContent->loadFromData(mData)) {
                qWarning() << "Unable to use preview of " << q->document()->url().fileName();
                return false;
            }
            // Use the size from JpegContent, as its correctly transposed if the
            // image has been rotated
            mImageSize = mJpegContent->size();

            mCmsProfile = Cms::Profile::loadFromExiv2Image(mExiv2Image.get());

        }

        LOG("mImageSize" << mImageSize);

        if (!mCmsProfile) {
            mCmsProfile = Cms::Profile::loadFromImageData(mData, mFormat);
        }

        return true;
    }

    void loadImageData()
    {
        QBuffer buffer;
        buffer.setBuffer(&mData);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, mFormat);

        LOG("mImageDataInvertedZoom=" << mImageDataInvertedZoom);
        if (mImageSize.isValid()
                && mImageDataInvertedZoom != 1
                && reader.supportsOption(QImageIOHandler::ScaledSize)
           ) {
            // Do not use mImageSize here: QImageReader needs a non-transposed
            // image size
            QSize size = reader.size() / mImageDataInvertedZoom;
            if (!size.isEmpty()) {
                LOG("Setting scaled size to" << size);
                reader.setScaledSize(size);
            } else {
                LOG("Not setting scaled size as it is empty" << size);
            }
        }

        if (GwenviewConfig::applyExifOrientation()) {
            reader.setAutoTransform(true);
        }

        bool ok = reader.read(&mImage);
        if (!ok) {
            LOG("QImageReader::read() failed");
            return;
        }

        if (reader.supportsAnimation()
                && reader.nextImageDelay() > 0 // Assume delay == 0 <=> only one frame
           ) {
            /*
             * QImageReader is not really helpful to detect animated gif:
             * - QImageReader::imageCount() returns 0
             * - QImageReader::nextImageDelay() may return something > 0 if the
             *   image consists of only one frame but includes a "Graphic
             *   Control Extension" (usually only present if we have an
             *   animation) (Bug #185523)
             *
             * Decoding the next frame is the only reliable way I found to
             * detect an animated gif
             */
            LOG("May be an animated image. delay:" << reader.nextImageDelay());
            QImage nextImage;
            if (reader.read(&nextImage)) {
                LOG("Really an animated image (more than one frame)");
                mAnimated = true;
            } else {
                qWarning() << q->document()->url() << "is not really an animated image (only one frame)";
            }
        }
    }
};

LoadingDocumentImpl::LoadingDocumentImpl(Document* document)
: AbstractDocumentImpl(document)
, d(new LoadingDocumentImplPrivate)
{
    d->q = this;
    d->mMetaInfoLoaded = false;
    d->mAnimated = false;
    d->mDownSampledImageLoaded = false;
    d->mImageDataInvertedZoom = 0;

    connect(&d->mMetaInfoFutureWatcher, &QFutureWatcherBase::finished,
            this, &LoadingDocumentImpl::slotMetaInfoLoaded);

    connect(&d->mImageDataFutureWatcher, &QFutureWatcherBase::finished,
            this, &LoadingDocumentImpl::slotImageLoaded);
}

LoadingDocumentImpl::~LoadingDocumentImpl()
{
    LOG("");
    // Disconnect watchers to make sure they do not trigger further work
    d->mMetaInfoFutureWatcher.disconnect();
    d->mImageDataFutureWatcher.disconnect();

    d->mMetaInfoFutureWatcher.waitForFinished();
    d->mImageDataFutureWatcher.waitForFinished();

    if (d->mTransferJob) {
        d->mTransferJob->kill();
    }
    delete d;
}

void LoadingDocumentImpl::init()
{
    QUrl url = document()->url();

    if (UrlUtils::urlIsFastLocalFile(url)) {
        // Load file content directly
        QFile file(url.toLocalFile());
        if (!file.open(QIODevice::ReadOnly)) {
            setDocumentErrorString(i18nc("@info", "Could not open file %1", url.toLocalFile()));
            emit loadingFailed();
            switchToImpl(new EmptyDocumentImpl(document()));
            return;
        }
        d->mData = file.read(HEADER_SIZE);
        if (d->determineKind()) {
            return;
        }
        d->mData += file.readAll();
        d->startLoading();
    } else {
        // Transfer file via KIO
        d->mTransferJob = KIO::get(document()->url(), KIO::NoReload, KIO::HideProgressInfo);
        connect(d->mTransferJob.data(), &KIO::TransferJob::data,
                this, &LoadingDocumentImpl::slotDataReceived);
        connect(d->mTransferJob.data(), &KJob::result,
                this, &LoadingDocumentImpl::slotTransferFinished);
        d->mTransferJob->start();
    }
}

void LoadingDocumentImpl::loadImage(int invertedZoom)
{
    if (d->mImageDataInvertedZoom == invertedZoom) {
        LOG("Already loading an image at invertedZoom=" << invertedZoom);
        return;
    }
    if (d->mImageDataInvertedZoom == 1) {
        LOG("Ignoring request: we are loading a full image");
        return;
    }
    d->mImageDataFutureWatcher.waitForFinished();
    d->mImageDataInvertedZoom = invertedZoom;

    if (d->mMetaInfoLoaded) {
        // Do not test on mMetaInfoFuture.isRunning() here: it might not have
        // started if we are downloading the image from a remote url
        d->startImageDataLoading();
    }
}

void LoadingDocumentImpl::slotDataReceived(KIO::Job* job, const QByteArray& chunk)
{
    d->mData.append(chunk);
    if (document()->kind() == MimeTypeUtils::KIND_UNKNOWN && d->mData.length() >= HEADER_SIZE) {
        if (d->determineKind()) {
            job->kill();
            return;
        }
    }
}

void LoadingDocumentImpl::slotTransferFinished(KJob* job)
{
    if (job->error()) {
        setDocumentErrorString(job->errorString());
        emit loadingFailed();
        switchToImpl(new EmptyDocumentImpl(document()));
        return;
    }
    d->startLoading();
}

bool LoadingDocumentImpl::isEditable() const
{
    return d->mDownSampledImageLoaded;
}

Document::LoadingState LoadingDocumentImpl::loadingState() const
{
    if (!document()->image().isNull()) {
        return Document::Loaded;
    } else if (d->mMetaInfoLoaded) {
        return Document::MetaInfoLoaded;
    } else if (document()->kind() != MimeTypeUtils::KIND_UNKNOWN) {
        return Document::KindDetermined;
    } else {
        return Document::Loading;
    }
}

void LoadingDocumentImpl::slotMetaInfoLoaded()
{
    LOG("");
    Q_ASSERT(!d->mMetaInfoFuture.isRunning());
    if (!d->mMetaInfoFuture.result()) {
        setDocumentErrorString(
            i18nc("@info", "Loading meta information failed.")
        );
        emit loadingFailed();
        switchToImpl(new EmptyDocumentImpl(document()));
        return;
    }

    setDocumentFormat(d->mFormat);
    setDocumentImageSize(d->mImageSize);
    setDocumentExiv2Image(std::move(d->mExiv2Image));
    setDocumentCmsProfile(d->mCmsProfile);

    d->mMetaInfoLoaded = true;
    emit metaInfoLoaded();

    // Start image loading if necessary
    // We test if mImageDataFuture is not already running because code connected to
    // metaInfoLoaded() signal could have called loadImage()
    if (!d->mImageDataFuture.isRunning() && d->mImageDataInvertedZoom != 0) {
        d->startImageDataLoading();
    }
}

void LoadingDocumentImpl::slotImageLoaded()
{
    LOG("");
    if (d->mImage.isNull()) {
        setDocumentErrorString(
            i18nc("@info", "Loading image failed.")
        );
        emit loadingFailed();
        switchToImpl(new EmptyDocumentImpl(document()));
        return;
    }

    if (d->mAnimated) {
        if (d->mImage.size() == d->mImageSize) {
            // We already decoded the first frame at the right size, let's show
            // it
            setDocumentImage(d->mImage);
        }

        switchToImpl(new AnimatedDocumentLoadedImpl(
                         document(),
                         d->mData));

        return;
    }

    if (d->mImageDataInvertedZoom != 1 && d->mImage.size() != d->mImageSize) {
        LOG("Loaded a down sampled image");
        d->mDownSampledImageLoaded = true;
        // We loaded a down sampled image
        setDocumentDownSampledImage(d->mImage, d->mImageDataInvertedZoom);
        return;
    }

    LOG("Loaded a full image");
    setDocumentImage(d->mImage);
    DocumentLoadedImpl* impl;
    if (d->mJpegContent.get()) {
        impl = new JpegDocumentLoadedImpl(
            document(),
            d->mJpegContent.release());
    } else {
        impl = new DocumentLoadedImpl(
            document(),
            d->mData);
    }
    switchToImpl(impl);
}

} // namespace
