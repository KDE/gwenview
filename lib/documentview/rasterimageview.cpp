// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#include "rasterimageview.h"

// Local
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/imagescaler.h>
#include <lib/cms/cmsprofile.h>
#include <lib/gvdebug.h>
#include <lib/paintutils.h>
#include "gwenview_lib_debug.h"

// KF

// Qt
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QPointer>
#include <QApplication>


namespace Gwenview
{

struct RasterImageViewPrivate
{
    RasterImageView* q;
    ImageScaler* mScaler;
    bool mEmittedCompleted;

    // Config
    AbstractImageView::AlphaBackgroundMode mAlphaBackgroundMode;
    QColor mAlphaBackgroundColor;
    cmsUInt32Number mRenderingIntent;
    // /Config

    bool mBufferIsEmpty;
    QPixmap mCurrentBuffer;
    // The alternate buffer is useful when scrolling: existing content is copied
    // to mAlternateBuffer and buffers are swapped. This avoids allocating a new
    // QPixmap every time the image is scrolled.
    QPixmap mAlternateBuffer;

    QTimer* mUpdateTimer;

    QPointer<AbstractRasterImageViewTool> mTool;

    bool mApplyDisplayTransform; // Defaults to true. Can be set to false if there is no need or no way to apply color profile
    cmsHTRANSFORM mDisplayTransform;

    void updateDisplayTransform(QImage::Format format)
    {
        GV_RETURN_IF_FAIL(format != QImage::Format_Invalid);
        mApplyDisplayTransform = false;
        if (mDisplayTransform) {
            cmsDeleteTransform(mDisplayTransform);
        }
        mDisplayTransform = nullptr;

        Cms::Profile::Ptr profile = q->document()->cmsProfile();
        if (!profile) {
            // The assumption that something unmarked is *probably* sRGB is better than failing to apply any transform when one
            // has a wide-gamut screen.
            profile = Cms::Profile::getSRgbProfile();
        }
        Cms::Profile::Ptr monitorProfile = Cms::Profile::getMonitorProfile();
        if (!monitorProfile) {
            qCWarning(GWENVIEW_LIB_LOG) << "Could not get monitor color profile";
            return;
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
        default:
            qCWarning(GWENVIEW_LIB_LOG) << "Gwenview can only apply color profile on RGB32 or ARGB32 images";
            return;
        }

        mDisplayTransform = cmsCreateTransform(profile->handle(), cmsFormat,
                                               monitorProfile->handle(), cmsFormat,
                                               mRenderingIntent, cmsFLAGS_BLACKPOINTCOMPENSATION);
        mApplyDisplayTransform = true;
    }

    void setupUpdateTimer()
    {
        mUpdateTimer = new QTimer(q);
        mUpdateTimer->setInterval(500);
        mUpdateTimer->setSingleShot(true);
        QObject::connect(mUpdateTimer, SIGNAL(timeout()), q, SLOT(updateBuffer()));
    }

    void startAnimationIfNecessary()
    {
        if (q->document() && q->isVisible()) {
            q->document()->startAnimation();
        }
    }

    QRectF mapViewportToZoomedImage(const QRectF& viewportRect) const
    {
        return QRectF(
                   viewportRect.topLeft() - q->imageOffset() + q->scrollPos(),
                   viewportRect.size()
               );
    }

    void setScalerRegionToVisibleRect()
    {
        QRectF rect = mapViewportToZoomedImage(q->boundingRect());
        mScaler->setDestinationRegion(QRegion(rect.toRect()));
    }

    void resizeBuffer()
    {
        const auto dpr = q->devicePixelRatio();
        QSize size = q->visibleImageSize().toSize();
        if (size * dpr == mCurrentBuffer.size()) {
            return;
        }
        if (!size.isValid()) {
            mAlternateBuffer = QPixmap();
            mCurrentBuffer = QPixmap();
            return;
        }

        mAlternateBuffer = QPixmap(size * dpr);
        mAlternateBuffer.setDevicePixelRatio(dpr);
        mAlternateBuffer.fill(Qt::transparent);
        {
            QPainter painter(&mAlternateBuffer);
            painter.drawPixmap(0, 0, mCurrentBuffer);
        }
        qSwap(mAlternateBuffer, mCurrentBuffer);

        mAlternateBuffer = QPixmap();
    }

    void drawAlphaBackground(QPainter* painter, const QRect& viewportRect, const QPoint& zoomedImageTopLeft, const QPixmap &texture)
    {
        switch (mAlphaBackgroundMode) {
            case AbstractImageView::AlphaBackgroundNone:
                painter->fillRect(viewportRect, Qt::transparent);
                break;
            case AbstractImageView::AlphaBackgroundCheckBoard:
            {
                const QPoint textureOffset(
                    zoomedImageTopLeft.x() % texture.width(),
                    zoomedImageTopLeft.y() % texture.height());
                painter->drawTiledPixmap(
                    viewportRect,
                    texture,
                    textureOffset);
                break;
            }
            case AbstractImageView::AlphaBackgroundSolid:
                painter->fillRect(viewportRect, mAlphaBackgroundColor);
                break;
            default:
                Q_ASSERT(0);
        }
    }
};

RasterImageView::RasterImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, d(new RasterImageViewPrivate)
{
    d->q = this;
    d->mEmittedCompleted = false;
    d->mApplyDisplayTransform = true;
    d->mDisplayTransform = nullptr;

    d->mAlphaBackgroundMode = AlphaBackgroundNone;
    d->mAlphaBackgroundColor = Qt::black;
    d->mRenderingIntent = INTENT_PERCEPTUAL;

    d->mBufferIsEmpty = true;
    d->mScaler = new ImageScaler(this);
    connect(d->mScaler, &ImageScaler::scaledRect, this, &RasterImageView::updateFromScaler);

    d->setupUpdateTimer();
}

RasterImageView::~RasterImageView()
{
    if (d->mTool) {
        d->mTool.data()->toolDeactivated();
    }
    if (d->mDisplayTransform) {
        cmsDeleteTransform(d->mDisplayTransform);
    }
    delete d;
}

void RasterImageView::setAlphaBackgroundMode(AlphaBackgroundMode mode)
{
    d->mAlphaBackgroundMode = mode;
    if (document() && document()->hasAlphaChannel()) {
        d->mCurrentBuffer = QPixmap();
        updateBuffer();
    }
}

void RasterImageView::setAlphaBackgroundColor(const QColor& color)
{
    d->mAlphaBackgroundColor = color;
    if (document() && document()->hasAlphaChannel()) {
        d->mCurrentBuffer = QPixmap();
        updateBuffer();
    }
}

void RasterImageView::setRenderingIntent(const RenderingIntent::Enum& renderingIntent)
{
    if (d->mRenderingIntent != renderingIntent) {
        d->mRenderingIntent = renderingIntent;
        updateBuffer();
    }
}

void RasterImageView::resetMonitorICC()
{
    d->mApplyDisplayTransform = true;
    updateBuffer();
}

void RasterImageView::loadFromDocument()
{
    Document::Ptr doc = document();
    if (!doc) {
        return;
    }

    connect(doc.data(), &Document::metaInfoLoaded,
            this, &RasterImageView::slotDocumentMetaInfoLoaded);
    connect(doc.data(), &Document::isAnimatedUpdated,
            this, &RasterImageView::slotDocumentIsAnimatedUpdated);

    const Document::LoadingState state = doc->loadingState();
    if (state == Document::MetaInfoLoaded || state == Document::Loaded) {
        slotDocumentMetaInfoLoaded();
    }
}

void RasterImageView::slotDocumentMetaInfoLoaded()
{
    if (document()->size().isValid()) {
        QMetaObject::invokeMethod(this, &RasterImageView::finishSetDocument, Qt::QueuedConnection);
    } else {
        // Could not retrieve image size from meta info, we need to load the
        // full image now.
        connect(document().data(), &Document::loaded,
                this, &RasterImageView::finishSetDocument);
        document()->startLoadingFullImage();
    }
}

void RasterImageView::finishSetDocument()
{
    GV_RETURN_IF_FAIL(document()->size().isValid());

    d->mScaler->setDocument(document());
    d->resizeBuffer();
    applyPendingScrollPos();

    connect(document().data(), &Document::imageRectUpdated,
            this, &RasterImageView::updateImageRect);

    if (zoomToFit()) {
        // Force the update otherwise if computeZoomToFit() returns 1, setZoom()
        // will think zoom has not changed and won't update the image
        setZoom(computeZoomToFit(), QPointF(-1, -1), ForceUpdate);
    } else if (zoomToFill()) {
        setZoom(computeZoomToFill(), QPointF(-1, -1), ForceUpdate);
    } else {
        // Not only call updateBuffer, but also ensure the initial transformation mode
        // of the image scaler is set correctly when zoom is unchanged (see Bug 396736).
        onZoomChanged();
    }

    d->startAnimationIfNecessary();
    update();
}

void RasterImageView::updateImageRect(const QRect& imageRect)
{
    QRectF viewRect = mapToView(imageRect);
    if (!viewRect.intersects(boundingRect())) {
        return;
    }

    if (zoomToFit()) {
        setZoom(computeZoomToFit());
    } else if (zoomToFill()) {
        setZoom(computeZoomToFill());
    } else {
        applyPendingScrollPos();
    }

    d->setScalerRegionToVisibleRect();
    update();
    emit imageRectUpdated();
}

void RasterImageView::slotDocumentIsAnimatedUpdated()
{
    d->startAnimationIfNecessary();
}

void RasterImageView::updateFromScaler(int zoomedImageLeft, int zoomedImageTop, const QImage& image)
{
    if (d->mApplyDisplayTransform) {
        d->updateDisplayTransform(image.format());
        if (d->mDisplayTransform) {
            quint8 *bytes = const_cast<quint8*>(image.bits());
            cmsDoTransform(d->mDisplayTransform, bytes, bytes, image.width() * image.height());
        }
    }

    d->resizeBuffer();
    int viewportLeft = zoomedImageLeft - scrollPos().x();
    int viewportTop = zoomedImageTop - scrollPos().y();
    d->mBufferIsEmpty = false;
    {
        QPainter painter(&d->mCurrentBuffer);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        if (document()->hasAlphaChannel()) {
            d->drawAlphaBackground(
                &painter, QRect(viewportLeft, viewportTop, image.width(), image.height()),
                QPoint(zoomedImageLeft, zoomedImageTop),
                alphaBackgroundTexture()
            );
            // This is required so transparent pixels don't replace our background
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        }
        painter.drawImage(viewportLeft, viewportTop, image);
    }
    update();

    if (!d->mEmittedCompleted) {
        d->mEmittedCompleted = true;
        emit completed();
    }
}

void RasterImageView::onZoomChanged()
{
    d->mScaler->setZoom(zoom());
    if (!d->mUpdateTimer->isActive()) {
        updateBuffer();
    }
}

void RasterImageView::onImageOffsetChanged()
{
    update();
}

void RasterImageView::onScrollPosChanged(const QPointF& oldPos)
{
    QPointF delta = scrollPos() - oldPos;

    // Scroll existing
    {
        if (d->mAlternateBuffer.size() != d->mCurrentBuffer.size()) {
            d->mAlternateBuffer = QPixmap(d->mCurrentBuffer.size());
            d->mAlternateBuffer.setDevicePixelRatio(d->mCurrentBuffer.devicePixelRatio());
        }
        d->mAlternateBuffer.fill(Qt::transparent);
        QPainter painter(&d->mAlternateBuffer);
        painter.drawPixmap(-delta, d->mCurrentBuffer);
    }
    qSwap(d->mCurrentBuffer, d->mAlternateBuffer);

    // Scale missing parts
    QRegion bufferRegion = QRect(scrollPos().toPoint(), d->mCurrentBuffer.size() / devicePixelRatio());
    QRegion updateRegion = bufferRegion - bufferRegion.translated(-delta.toPoint());
    updateBuffer(updateRegion);
    update();
}

void RasterImageView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    d->mCurrentBuffer.setDevicePixelRatio(devicePixelRatio());

    QPointF topLeft = imageOffset();
    if (zoomToFit()) {
        // In zoomToFit mode, scale crudely the buffer to fit the screen. This
        // provide an approximate rendered which will be replaced when the scheduled
        // proper scale is ready.
        // Round point and size independently, to keep consistency with the below (non zoomToFit) painting
        const QRect rect = QRect(topLeft.toPoint(), (dipDocumentSize() * zoom()).toSize());
        painter->drawPixmap(rect, d->mCurrentBuffer);
    } else {
        painter->drawPixmap(topLeft.toPoint(), d->mCurrentBuffer);
    }

    if (d->mTool) {
        d->mTool.data()->paint(painter);
    }

    // Debug
#if 0
    QSizeF visibleSize = documentSize() * zoom();
    painter->setPen(Qt::red);
    painter->drawRect(topLeft.x(), topLeft.y(), visibleSize.width() - 1, visibleSize.height() - 1);

    painter->setPen(Qt::blue);
    painter->drawRect(topLeft.x(), topLeft.y(), d->mCurrentBuffer.width() - 1, d->mCurrentBuffer.height() - 1);
#endif
}

void RasterImageView::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    // If we are in zoomToFit mode and have something in our buffer, delay the
    // update: paint() will paint a scaled version of the buffer until resizing
    // is done. This is much faster than rescaling the whole image for each
    // resize event we receive.
    // mUpdateTimer must be started before calling AbstractImageView::resizeEvent()
    // because AbstractImageView::resizeEvent() will call onZoomChanged(), which
    // will trigger an immediate update unless the mUpdateTimer is active.
    if ((zoomToFit() || zoomToFill()) && !d->mBufferIsEmpty) {
        d->mUpdateTimer->start();
    }
    AbstractImageView::resizeEvent(event);
    if (!zoomToFit() || !zoomToFill()) {
        // Only update buffer if we are not in zoomToFit mode: if we are
        // onZoomChanged() will have already updated the buffer.
        updateBuffer();
    }
}

void RasterImageView::updateBuffer(const QRegion& region)
{
    d->mUpdateTimer->stop();
    if (region.isEmpty()) {
        d->setScalerRegionToVisibleRect();
    } else {
        d->mScaler->setDestinationRegion(region);
    }
}

void RasterImageView::setCurrentTool(AbstractRasterImageViewTool* tool)
{
    if (d->mTool) {
        d->mTool.data()->toolDeactivated();
        d->mTool.data()->deleteLater();
    }

    // Go back to default cursor when tool is deactivated. We need to call this here and
    // not further below in case toolActivated wants to set its own new cursor afterwards.
    updateCursor();

    d->mTool = tool;
    if (d->mTool) {
        d->mTool.data()->toolActivated();
    }
    emit currentToolChanged(tool);
    update();
}

AbstractRasterImageViewTool* RasterImageView::currentTool() const
{
    return d->mTool.data();
}

void RasterImageView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mousePressEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mousePressEvent(event);
}

void RasterImageView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mouseDoubleClickEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseDoubleClickEvent(event);
}

void RasterImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mouseMoveEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseMoveEvent(event);
}

void RasterImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->mouseReleaseEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseReleaseEvent(event);
}

void RasterImageView::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->wheelEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::wheelEvent(event);
}

void RasterImageView::keyPressEvent(QKeyEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->keyPressEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::keyPressEvent(event);
}

void RasterImageView::keyReleaseEvent(QKeyEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->keyReleaseEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::keyReleaseEvent(event);
}

void RasterImageView::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (d->mTool) {
        d->mTool.data()->hoverMoveEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::hoverMoveEvent(event);
}

} // namespace
