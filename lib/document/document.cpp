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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "document.h"
#include "document_p.h"

// Qt
#include <QApplication>
#include <QImage>
#include <QUndoStack>
#include <QUrl>
#include <QtConcurrentRun>

// KF
#include <KJobUiDelegate>
#include <KLocalizedString>

// Exiv2
#include <exiv2/exiv2.hpp>

// Local
#include "documentjob.h"
#include "gvdebug.h"
#include "gwenview_lib_debug.h"
#include "gwenviewconfig.h"
#include "imagemetainfomodel.h"
#include "loadingdocumentimpl.h"
#include "loadingjob.h"
#include "savejob.h"

namespace Gwenview
{
#undef ENABLE_LOG
#undef LOG
// #define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) // qCDebug(GWENVIEW_LIB_LOG) << x
#else
#define LOG(x) ;
#endif

#ifdef ENABLE_LOG

static void logQueue(DocumentPrivate *d)
{
#define PREFIX "  QUEUE: "
    if (!d->mCurrentJob) {
        Q_ASSERT(d->mJobQueue.isEmpty());
        qDebug(PREFIX "No current job, no pending jobs");
        return;
    }
    qCDebug(GWENVIEW_LIB_LOG) << PREFIX "Current job:" << d->mCurrentJob.data();
    if (d->mJobQueue.isEmpty()) {
        qDebug(PREFIX "No pending jobs");
        return;
    }
    qDebug(PREFIX "%d pending job(s):", d->mJobQueue.size());
    for (DocumentJob *job : qAsConst(d->mJobQueue)) {
        Q_ASSERT(job);
        qCDebug(GWENVIEW_LIB_LOG) << PREFIX "-" << job;
    }
#undef PREFIX
}

#define LOG_QUEUE(msg, d)                                                                                                                                      \
    LOG(msg);                                                                                                                                                  \
    logQueue(d)

#else

#define LOG_QUEUE(msg, d)

#endif

namespace
{

cmsHTRANSFORM createDisplayTransform(QImage::Format format, Cms::Profile::Ptr profile, cmsUInt32Number renderingIntent)
{
    if (format == QImage::Format_Invalid) {
        return {};
    }

    if (!profile) {
        // The assumption that something unmarked is *probably* sRGB is better than failing to apply any transform when one
        // has a wide-gamut screen.
        profile = Cms::Profile::getSRgbProfile();
    }
    Cms::Profile::Ptr monitorProfile = Cms::Profile::getMonitorProfile();
    if (!monitorProfile) {
        qCWarning(GWENVIEW_LIB_LOG) << "Could not get monitor color profile";
        return {};
    }

    cmsUInt32Number cmsFormat = 0;
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        cmsFormat = TYPE_BGRA_8;
        break;
    case QImage::Format_Grayscale8:
        cmsFormat = TYPE_GRAY_8;
        break;
    case QImage::Format_RGB888:
        cmsFormat = TYPE_RGB_8;
        break;
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        cmsFormat = TYPE_RGBA_8;
        break;
    case QImage::Format_Grayscale16:
        cmsFormat = TYPE_GRAY_16;
        break;
    case QImage::Format_RGBA64:
    case QImage::Format_RGBX64:
        cmsFormat = TYPE_RGBA_16;
        break;
    case QImage::Format_BGR888:
        cmsFormat = TYPE_BGR_8;
        break;
    default:
        qCWarning(GWENVIEW_LIB_LOG) << "Gwenview cannot apply color profile on" << format << "images";
        return {};
    }

    return cmsCreateTransform(profile->handle(), cmsFormat, monitorProfile->handle(), cmsFormat, renderingIntent, cmsFLAGS_BLACKPOINTCOMPENSATION);
}

void applyDisplayTransform(QImage &image, Cms::Profile::Ptr const &profile, cmsUInt32Number renderingIntent)
{
    auto transform = createDisplayTransform(image.format(), profile, renderingIntent);
    if (!transform)
        return;
    quint8 *bytes = image.bits();
    cmsDoTransform(transform, bytes, bytes, image.width() * image.height());
    cmsDeleteTransform(transform);
}

}

//- DocumentPrivate ---------------------------------------
void DocumentPrivate::scheduleImageLoading(int invertedZoom)
{
    auto impl = qobject_cast<LoadingDocumentImpl *>(mImpl);
    Q_ASSERT(impl);
    impl->loadImage(invertedZoom);
}

void DocumentPrivate::scheduleImageDownSampling(int invertedZoom)
{
    LOG("invertedZoom=" << invertedZoom);
    auto job = qobject_cast<DownSamplingJob *>(mCurrentJob.data());
    if (job && job->mInvertedZoom == invertedZoom) {
        LOG("Current job is already doing it");
        return;
    }

    // Remove any previously scheduled downsampling job
    DocumentJobQueue::Iterator it;
    for (it = mJobQueue.begin(); it != mJobQueue.end(); ++it) {
        auto job = qobject_cast<DownSamplingJob *>(*it);
        if (!job) {
            continue;
        }
        if (job->mInvertedZoom == invertedZoom) {
            // Already scheduled, nothing to do
            LOG("Already scheduled");
            return;
        } else {
            LOG("Removing downsampling job");
            mJobQueue.erase(it);
            delete job;
        }
    }
    q->enqueueJob(new DownSamplingJob(invertedZoom));
}

void DocumentPrivate::scheduleImageColorCorrection()
{
    auto *job = qobject_cast<ColorCorrectionJob *>(mCurrentJob.data());
    if (job) {
        return;
    }

    if (std::any_of(mJobQueue.begin(), mJobQueue.end(), [](auto *j) {
            return qobject_cast<ColorCorrectionJob *>(j);
        }))
        return;

    q->enqueueJob(new ColorCorrectionJob());
}

void DocumentPrivate::downSampleImage(int invertedZoom)
{
    mDownSampledImageMap[invertedZoom] = mImage.scaled(mImage.size() / invertedZoom, Qt::KeepAspectRatio, Qt::FastTransformation);
    if (mDownSampledImageMap[invertedZoom].size().isEmpty()) {
        mDownSampledImageMap[invertedZoom] = mImage;
    }
    Q_EMIT q->downSampledImageReady();
}

//- DownSamplingJob ---------------------------------------
void DownSamplingJob::doStart()
{
    DocumentPrivate *d = document()->d;
    d->downSampleImage(mInvertedZoom);
    setError(NoError);
    emitResult();
}

ColorCorrectionJob::ColorCorrectionJob()
{
    QObject::connect(&mWatcher, &QFutureWatcher<QImage>::finished, this, [this]() {
        DocumentPrivate *d = document()->d;
        d->mColorCorrectedImage = mWatcher.future().result();
        setError(NoError);
        emitResult();
    });
}

void ColorCorrectionJob::doStart()
{
    DocumentPrivate *d = document()->d;
    mWatcher.setFuture(QtConcurrent::run([image = d->mImage, profile = d->mCmsProfile, renderingIntent = d->mRenderingIntent]() {
        const bool isIndexedColor = image.colorCount() > 0;

        QImage::Format outputImageFormat = image.format();
        if (isIndexedColor) {
            outputImageFormat = image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
        } else {
            switch (outputImageFormat) {
            case QImage::Format_ARGB32_Premultiplied:
                outputImageFormat = QImage::Format_ARGB32;
                break;
            case QImage::Format_RGBA8888_Premultiplied:
                outputImageFormat = QImage::Format_RGBA8888;
                break;
            case QImage::Format_RGBA64_Premultiplied:
                outputImageFormat = QImage::Format_RGBA64;
                break;
            // TODO convert formats not supported by LittleCMS?
            default:;
            }
        }

        auto colorCorrectedImage = image.copy();
        // We may load an image in indexed color or premultiplied alpha, or scaling may produce premultiplied alpha.
        // These are not supported by the color correction engine, so convert to a standard format.
        if (colorCorrectedImage.format() != outputImageFormat) {
            colorCorrectedImage.convertTo(outputImageFormat);
        }

        applyDisplayTransform(colorCorrectedImage, profile, renderingIntent);

        return colorCorrectedImage;
    }));
}

ColorCorrectionJob::~ColorCorrectionJob()
{
    mWatcher.waitForFinished();
}

//- Document ----------------------------------------------
qreal Document::maxDownSampledZoom()
{
    return 0.5;
}

Document::Document(const QUrl &url)
    : QObject()
    , d(new DocumentPrivate)
{
    d->q = this;
    d->mImpl = nullptr;
    d->mUrl = url;
    d->mKeepRawData = false;
}

Document::~Document()
{
    // We do not want undo stack to emit signals, forcing us to emit signals
    // ourself while we are being destroyed.
    disconnect(&d->mUndoStack, nullptr, this, nullptr);

    delete d->mImpl;
    delete d;
}

void Document::reload()
{
    d->mSize = QSize();
    d->mImage = QImage();
    d->mColorCorrectedImage = QImage();
    d->mDownSampledImageMap.clear();
    d->mExiv2Image.reset();
    d->mKind = MimeTypeUtils::KIND_UNKNOWN;
    d->mFormat = QByteArray();
    d->mImageMetaInfoModel.setUrl(d->mUrl);
    d->mImageMetaInfoModel.setDates(d->mUrl);
    d->mImageMetaInfoModel.setMimeType(d->mUrl);
    d->mImageMetaInfoModel.setFileSize(d->mUrl);
    d->mUndoStack.clear();
    d->mErrorString.clear();
    d->mCmsProfile = nullptr;
    d->mRenderingIntent = GwenviewConfig::renderingIntent();

    switchToImpl(new LoadingDocumentImpl(this));
}

void Document::setRenderingIntent(RenderingIntent::Enum intent)
{
    d->mRenderingIntent = intent;
}

const QImage &Document::image() const
{
    return d->mImage;
}

QImage Document::colorCorrectedImage() const
{
    if (!d->mApplyDisplayTransform) {
        return d->mImage;
    }

    if (d->mColorCorrectedImage.isNull()) {
        // This is just a fallback for the case the job hasn't finished yet
        auto ret = d->mImage.copy();
        applyDisplayTransform(ret, d->mCmsProfile, d->mRenderingIntent);
        d->mColorCorrectedImage = ret;
        return ret;
    }
    return d->mColorCorrectedImage;
}

/**
 * invertedZoom is the biggest power of 2 for which zoom < 1/invertedZoom.
 * Example:
 * zoom = 0.4 == 1/2.5 => invertedZoom = 2 (1/2.5 < 1/2)
 * zoom = 0.2 == 1/5   => invertedZoom = 4 (1/5   < 1/4)
 */
inline int invertedZoomForZoom(qreal zoom)
{
    int invertedZoom;
    for (invertedZoom = 1; zoom < 1. / (invertedZoom * 4); invertedZoom *= 2) { }
    return invertedZoom;
}

const QImage &Document::downSampledImageForZoom(qreal zoom) const
{
    static const QImage sNullImage;

    int invertedZoom = invertedZoomForZoom(zoom);
    if (invertedZoom == 1) {
        return d->mImage;
    }

    if (!d->mDownSampledImageMap.contains(invertedZoom)) {
        if (!d->mImage.isNull()) {
            // Special case: if we have the full image and the down sampled
            // image would be too small, return the original image.
            const QSize downSampledSize = d->mImage.size() / invertedZoom;
            if (downSampledSize.isEmpty()) {
                return d->mImage;
            }
        }
        return sNullImage;
    }

    return d->mDownSampledImageMap[invertedZoom];
}

Document::LoadingState Document::loadingState() const
{
    return d->mImpl->loadingState();
}

void Document::switchToImpl(AbstractDocumentImpl *impl)
{
    Q_ASSERT(impl);
    LOG("old impl:" << d->mImpl << "new impl:" << impl);
    if (d->mImpl) {
        d->mImpl->deleteLater();
    }
    d->mImpl = impl;

    connect(d->mImpl, &AbstractDocumentImpl::metaInfoLoaded, this, &Document::emitMetaInfoLoaded);
    connect(d->mImpl, &AbstractDocumentImpl::loaded, this, &Document::emitLoaded);
    connect(d->mImpl, &AbstractDocumentImpl::loaded, this, &Document::prepareColorCorrectedImage);
    connect(d->mImpl, &AbstractDocumentImpl::loadingFailed, this, &Document::emitLoadingFailed);
    connect(d->mImpl, &AbstractDocumentImpl::imageRectUpdated, this, &Document::imageRectUpdated);
    connect(d->mImpl, &AbstractDocumentImpl::isAnimatedUpdated, this, &Document::isAnimatedUpdated);
    d->mImpl->init();
}

void Document::setImageInternal(const QImage &image)
{
    d->mImage = image;
    d->mDownSampledImageMap.clear();

    // If we didn't get the image size before decoding the full image, set it
    // now
    setSize(d->mImage.size());
}

QUrl Document::url() const
{
    return d->mUrl;
}

QByteArray Document::rawData() const
{
    return d->mImpl->rawData();
}

bool Document::keepRawData() const
{
    return d->mKeepRawData;
}

void Document::setKeepRawData(bool value)
{
    d->mKeepRawData = value;
}

void Document::waitUntilLoaded()
{
    startLoadingFullImage();
    while (true) {
        LoadingState state = loadingState();
        if (state == Loaded || state == LoadingFailed) {
            return;
        }
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

DocumentJob *Document::save(const QUrl &url, const QByteArray &format)
{
    waitUntilLoaded();
    DocumentJob *job = d->mImpl->save(url, format);
    if (!job) {
        qCWarning(GWENVIEW_LIB_LOG) << "Implementation does not support saving!";
        setErrorString(i18nc("@info", "Gwenview cannot save this kind of documents."));
        return nullptr;
    }
    job->setProperty("oldUrl", d->mUrl);
    job->setProperty("newUrl", url);
    connect(job, &DocumentJob::result, this, &Document::slotSaveResult);
    enqueueJob(job);
    return job;
}

void Document::slotSaveResult(KJob *job)
{
    if (job->error()) {
        setErrorString(job->errorString());
    } else {
        d->mUndoStack.setClean();
        auto saveJob = static_cast<SaveJob *>(job);
        d->mUrl = saveJob->newUrl();
        d->mImageMetaInfoModel.setUrl(d->mUrl);
        d->mImageMetaInfoModel.setDates(d->mUrl);
        d->mImageMetaInfoModel.setFileSize(d->mUrl);
        Q_EMIT saved(saveJob->oldUrl(), d->mUrl);
    }
}

QByteArray Document::format() const
{
    return d->mFormat;
}

void Document::setFormat(const QByteArray &format)
{
    d->mFormat = format;
    Q_EMIT metaInfoUpdated();
}

MimeTypeUtils::Kind Document::kind() const
{
    return d->mKind;
}

void Document::setKind(MimeTypeUtils::Kind kind)
{
    d->mKind = kind;
    Q_EMIT kindDetermined(d->mUrl);
}

QSize Document::size() const
{
    return d->mSize;
}

bool Document::hasAlphaChannel() const
{
    if (d->mImage.isNull()) {
        return false;
    } else {
        return d->mImage.hasAlphaChannel();
    }
}

int Document::memoryUsage() const
{
    // FIXME: Take undo stack into account
    int usage = d->mImage.sizeInBytes();
    usage += rawData().length();
    return usage;
}

void Document::setSize(const QSize &size)
{
    if (size == d->mSize) {
        return;
    }
    d->mSize = size;
    d->mImageMetaInfoModel.setImageSize(size);
    Q_EMIT metaInfoUpdated();
}

bool Document::isModified() const
{
    return !d->mUndoStack.isClean();
}

AbstractDocumentEditor *Document::editor()
{
    return d->mImpl->editor();
}

void Document::setExiv2Image(std::unique_ptr<Exiv2::Image> image)
{
    d->mExiv2Image = std::move(image);
    d->mImageMetaInfoModel.setExiv2Image(d->mExiv2Image.get());
    Q_EMIT metaInfoUpdated();
}

void Document::setDownSampledImage(const QImage &image, int invertedZoom)
{
    Q_ASSERT(!d->mDownSampledImageMap.contains(invertedZoom));
    d->mDownSampledImageMap[invertedZoom] = image;
    Q_EMIT downSampledImageReady();
}

QString Document::errorString() const
{
    return d->mErrorString;
}

void Document::setErrorString(const QString &string)
{
    d->mErrorString = string;
}

ImageMetaInfoModel *Document::metaInfo() const
{
    return &d->mImageMetaInfoModel;
}

void Document::startLoadingFullImage()
{
    LoadingState state = loadingState();
    if (state <= MetaInfoLoaded) {
        // Schedule full image loading
        auto job = new LoadingJob;
        job->uiDelegate()->setAutoWarningHandlingEnabled(false);
        job->uiDelegate()->setAutoErrorHandlingEnabled(false);
        enqueueJob(job);
        d->scheduleImageLoading(1);
    } else if (state == Loaded) {
        return;
    } else if (state == LoadingFailed) {
        qCWarning(GWENVIEW_LIB_LOG) << "Can't load full image: loading has already failed";
    }
}

void Document::prepareColorCorrectedImage()
{
    d->scheduleImageColorCorrection();
}

bool Document::prepareDownSampledImageForZoom(qreal zoom)
{
    if (zoom >= maxDownSampledZoom()) {
        qCWarning(GWENVIEW_LIB_LOG) << "No need to call prepareDownSampledImageForZoom if zoom >= " << maxDownSampledZoom();
        return true;
    }

    int invertedZoom = invertedZoomForZoom(zoom);
    if (d->mDownSampledImageMap.contains(invertedZoom)) {
        LOG("downSampledImageForZoom=" << zoom << "invertedZoom=" << invertedZoom << "ready");
        return true;
    }

    LOG("downSampledImageForZoom=" << zoom << "invertedZoom=" << invertedZoom << "not ready");
    if (loadingState() == LoadingFailed) {
        qCWarning(GWENVIEW_LIB_LOG) << "Image has failed to load, not doing anything";
        return false;
    } else if (loadingState() == Loaded) {
        d->scheduleImageDownSampling(invertedZoom);
        return false;
    }

    // Schedule down sampled image loading
    d->scheduleImageLoading(invertedZoom);

    return false;
}

void Document::emitMetaInfoLoaded()
{
    Q_EMIT metaInfoLoaded(d->mUrl);
}

void Document::emitLoaded()
{
    Q_EMIT loaded(d->mUrl);
}

void Document::emitLoadingFailed()
{
    Q_EMIT loadingFailed(d->mUrl);
}

QUndoStack *Document::undoStack() const
{
    return &d->mUndoStack;
}

void Document::imageOperationCompleted()
{
    if (d->mUndoStack.isClean()) {
        // If user just undid all his changes this does not really correspond
        // to a save, but it's similar enough as far as Document users are
        // concerned
        Q_EMIT saved(d->mUrl, d->mUrl);
    } else {
        Q_EMIT modified(d->mUrl);
    }
}

bool Document::isEditable() const
{
    return d->mImpl->isEditable();
}

bool Document::isAnimated() const
{
    return d->mImpl->isAnimated();
}

void Document::startAnimation()
{
    return d->mImpl->startAnimation();
}

void Document::stopAnimation()
{
    return d->mImpl->stopAnimation();
}

void Document::enqueueJob(DocumentJob *job)
{
    LOG("job=" << job);
    job->setDocument(Ptr(this));
    connect(job, &LoadingJob::finished, this, &Document::slotJobFinished);
    if (d->mCurrentJob) {
        d->mJobQueue.enqueue(job);
    } else {
        d->mCurrentJob = job;
        LOG("Starting first job");
        job->start();
        Q_EMIT busyChanged(d->mUrl, true);
    }
    LOG_QUEUE("Job added", d);
}

void Document::slotJobFinished(KJob *job)
{
    LOG("job=" << job);
    GV_RETURN_IF_FAIL(job == d->mCurrentJob.data());

    if (d->mJobQueue.isEmpty()) {
        LOG("All done");
        d->mCurrentJob.clear();
        Q_EMIT busyChanged(d->mUrl, false);
        Q_EMIT allTasksDone();
    } else {
        LOG("Starting next job");
        d->mCurrentJob = d->mJobQueue.dequeue();
        GV_RETURN_IF_FAIL(d->mCurrentJob);
        d->mCurrentJob.data()->start();
    }
    LOG_QUEUE("Removed done job", d);
}

bool Document::isBusy() const
{
    return !d->mJobQueue.isEmpty();
}

QSvgRenderer *Document::svgRenderer() const
{
    return d->mImpl->svgRenderer();
}

void Document::setCmsProfile(const Cms::Profile::Ptr &ptr)
{
    d->mCmsProfile = ptr;
}

Cms::Profile::Ptr Document::cmsProfile() const
{
    return d->mCmsProfile;
}

} // namespace

#include "moc_document.cpp"

#include "moc_document_p.cpp"
