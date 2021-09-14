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
#include "alphabackgrounditem.h"
#include "gwenview_lib_debug.h"
#include "rasterimageitem.h"
#include <lib/cms/cmsprofile.h>
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/gvdebug.h>
#include <lib/paintutils.h>

// KF

// Qt
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QTimer>

#include <QColorSpace>
#include <QCryptographicHash>

namespace Gwenview
{
/*
 * We need the tools to be painted on top of the image. However, since we are
 * using RasterImageItem as a child of this item, the image gets painted on
 * top of this item. To fix that, this custom item is stacked after the image
 * item and will paint the tools, which will draw it on top of the image.
 */
class ToolPainter : public QGraphicsItem
{
public:
    ToolPainter(AbstractRasterImageViewTool *tool, QGraphicsItem *parent = nullptr)
        : QGraphicsItem(parent)
        , mTool(tool)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/) override
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

struct RasterImageViewPrivate {
    RasterImageViewPrivate(RasterImageView *qq)
        : q(qq)
    {
    }

    RasterImageView *q;

    RasterImageItem *mImageItem;
    ToolPainter *mToolItem = nullptr;

    QPointer<AbstractRasterImageViewTool> mTool;

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

RasterImageView::RasterImageView(QGraphicsItem *parent)
    : AbstractImageView(parent)
    , d(new RasterImageViewPrivate{this})
{
    d->mImageItem = new RasterImageItem{this};

    // Clip this item so we only render the visible part of the image when
    // zoomed or when viewing a large image.
    setFlag(QGraphicsItem::ItemClipsChildrenToShape);
}

RasterImageView::~RasterImageView()
{
    if (d->mTool) {
        d->mTool.data()->toolDeactivated();
    }

    delete d;
}

void RasterImageView::setRenderingIntent(const RenderingIntent::Enum &renderingIntent)
{
    d->mImageItem->setRenderingIntent(renderingIntent);
}

void RasterImageView::resetMonitorICC()
{
    update();
}

void RasterImageView::loadFromDocument()
{
    Document::Ptr doc = document();
    if (!doc) {
        return;
    }

    connect(doc.data(), &Document::metaInfoLoaded, this, &RasterImageView::slotDocumentMetaInfoLoaded);
    connect(doc.data(), &Document::isAnimatedUpdated, this, &RasterImageView::slotDocumentIsAnimatedUpdated);
    connect(doc.data(), &Document::imageRectUpdated, this, [this]() {
        d->mImageItem->updateCache();
    });

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
        connect(document().data(), &Document::loaded, this, &RasterImageView::finishSetDocument);
        document()->startLoadingFullImage();
    }
}

void RasterImageView::finishSetDocument()
{
    GV_RETURN_IF_FAIL(document()->size().isValid());

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

    d->mImageItem->updateCache();

    backgroundItem()->setVisible(true);

    Q_EMIT completed();
}

void RasterImageView::slotDocumentIsAnimatedUpdated()
{
    d->startAnimationIfNecessary();
}

void RasterImageView::onZoomChanged()
{
    d->adjustItemPosition();
}

void RasterImageView::onImageOffsetChanged()
{
    d->adjustItemPosition();
}

void RasterImageView::onScrollPosChanged(const QPointF &oldPos)
{
    Q_UNUSED(oldPos);
    d->adjustItemPosition();
}

void RasterImageView::setCurrentTool(AbstractRasterImageViewTool *tool)
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
    Q_EMIT currentToolChanged(tool);
    update();
}

AbstractRasterImageViewTool *RasterImageView::currentTool() const
{
    return d->mTool.data();
}

void RasterImageView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->mousePressEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mousePressEvent(event);
}

void RasterImageView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->mouseDoubleClickEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseDoubleClickEvent(event);
}

void RasterImageView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->mouseMoveEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseMoveEvent(event);
}

void RasterImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->mouseReleaseEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::mouseReleaseEvent(event);
}

void RasterImageView::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->wheelEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::wheelEvent(event);
}

void RasterImageView::keyPressEvent(QKeyEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->keyPressEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::keyPressEvent(event);
}

void RasterImageView::keyReleaseEvent(QKeyEvent *event)
{
    if (d->mTool) {
        d->mTool.data()->keyReleaseEvent(event);
        if (event->isAccepted()) {
            return;
        }
    }
    AbstractImageView::keyReleaseEvent(event);
}

void RasterImageView::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
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
