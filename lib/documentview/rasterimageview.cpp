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
#include <lib/cms/cmsprofile.h>
#include <lib/gvdebug.h>
#include <lib/paintutils.h>
#include "gwenview_lib_debug.h"
#include "alphabackgrounditem.h"

// KF

// Qt
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QPointer>
#include <QApplication>


namespace Gwenview
{

/*
 * This is effectively QGraphicsPixmapItem that draws the document's QImage
 * directly rather than going through a QPixmap intermediary. This avoids
 * duplicating the image contents and the extra memory usage that comes from
 * that.
 */
class RasterImageItem : public QGraphicsItem
{
public:
    RasterImageItem(RasterImageView* parent)
        : QGraphicsItem(parent)
        , mParentView(parent)
    {

    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) override
    {
        auto document = mParentView->document();

        // We want nearest neighbour when zooming in since that provides the most
        // accurate representation of pixels, but when zooming out it will actually
        // not look very nice, so use smoothing when zooming out.
        painter->setRenderHint(QPainter::SmoothPixmapTransform, mParentView->zoom() < 1.0);

        painter->drawImage(QPointF{0, 0}, document->image());
    }

    QRectF boundingRect() const override
    {
        return QRectF{QPointF{0, 0}, mParentView->documentSize()};
    }

    RasterImageView* mParentView;
};

/*
 * We need the tools to be painted on top of the image. However, since we are
 * using RasterImageItem as a child of this item, the image gets painted on
 * top of this item. To fix that, this custom item is stacked after the image
 * item and will paint the tools, which will draw it on top of the image.
 */
class ToolPainter : public QGraphicsItem
{
public:
    ToolPainter(AbstractRasterImageViewTool* tool, QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
        , mTool(tool)
    {

    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) override
    {
        if (mTool) {
            mTool->paint(painter);
        }
    }

    QRectF boundingRect() const override
    {
        return parentItem()->boundingRect();
    }

    QPointer<AbstractRasterImageViewTool> mTool;
};

struct RasterImageViewPrivate
{
    RasterImageViewPrivate(RasterImageView* qq)
        : q(qq)
    {
    }

    RasterImageView* q;

    QGraphicsItem* mImageItem;
    ToolPainter* mToolItem = nullptr;

    cmsUInt32Number mRenderingIntent = INTENT_PERCEPTUAL;

    QPointer<AbstractRasterImageViewTool> mTool;

    bool mApplyDisplayTransform = true; // Can be set to false if there is no need or no way to apply color profile
    cmsHTRANSFORM mDisplayTransform = nullptr;

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

    void applyImageTransform()
    {
        if (!q->document()) {
            return;
        }

        auto image = q->document()->image();

        if (mApplyDisplayTransform) {
            updateDisplayTransform(image.format());
            if (mDisplayTransform) {
                // This is really ugly, but to correctly change the image data
                // we need QImage to not detach. Simply using `image.bits()` will
                // not do that, so we need to const_cast as `constBits()` avoids
                // the detach.
                quint8 *bytes = const_cast<quint8*>(image.constBits());
                cmsDoTransform(mDisplayTransform, bytes, bytes, image.width() * image.height());
            }
        }

        q->update();
    }

    void startAnimationIfNecessary()
    {
        if (q->document() && q->isVisible()) {
            q->document()->startAnimation();
        }
    }

    void adjustItemPosition()
    {
        mImageItem->setPos((q->imageOffset() - q->scrollPos()).toPoint());
        q->update();
    }
};

RasterImageView::RasterImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, d(new RasterImageViewPrivate{this})
{
    d->mImageItem = new RasterImageItem{this};

    // Clip this item so we only render the visible part of the image when
    // zoomed or when viewing a large image.
    setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
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

void RasterImageView::setRenderingIntent(const RenderingIntent::Enum& renderingIntent)
{
    if (d->mRenderingIntent != renderingIntent) {
        d->mRenderingIntent = renderingIntent;
        d->applyImageTransform();
    }
}

void RasterImageView::resetMonitorICC()
{
    d->mApplyDisplayTransform = true;
    d->applyImageTransform();
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
    if (document()->size().isValid() && document()->image().format() != QImage::Format_Invalid) {
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

    d->applyImageTransform();

    if (zoomToFit()) {
        // Force the update otherwise if computeZoomToFit() returns 1, setZoom()
        // will think zoom has not changed and won't update the image
        setZoom(computeZoomToFit(), QPointF(-1, -1), ForceUpdate);
    } else if (zoomToFill()) {
        setZoom(computeZoomToFill(), QPointF(-1, -1), ForceUpdate);
    } else {
        onZoomChanged();
    }

    applyPendingScrollPos();
    d->startAnimationIfNecessary();
    update();

    backgroundItem()->setVisible(true);

    Q_EMIT completed();
}

void RasterImageView::slotDocumentIsAnimatedUpdated()
{
    d->startAnimationIfNecessary();
}

void RasterImageView::onZoomChanged()
{
    d->mImageItem->setScale(zoom() / devicePixelRatio());
    d->adjustItemPosition();
}

void RasterImageView::onImageOffsetChanged()
{
    d->adjustItemPosition();
}

void RasterImageView::onScrollPosChanged(const QPointF& oldPos)
{
    Q_UNUSED(oldPos);
    d->adjustItemPosition();
}

void RasterImageView::setCurrentTool(AbstractRasterImageViewTool* tool)
{
    if (d->mTool) {
        d->mTool.data()->toolDeactivated();
        d->mTool.data()->deleteLater();
        delete d->mToolItem;
    }

    // Go back to default cursor when tool is deactivated. We need to call this here and
    // not further below in case toolActivated wants to set its own new cursor afterwards.
    updateCursor();

    d->mTool = tool;
    if (d->mTool) {
        d->mTool.data()->toolActivated();
        d->mToolItem = new ToolPainter{d->mTool, this};
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
