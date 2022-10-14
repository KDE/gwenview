// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "svgviewadapter.h"

// Qt
#include <QCursor>
#include <QGraphicsSceneEvent>
#include <QGraphicsSvgItem>
#include <QGraphicsTextItem>
#include <QSvgRenderer>

// KF

// Local
#include "alphabackgrounditem.h"
#include "gwenview_lib_debug.h"
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>

namespace Gwenview
{
/// SvgImageView ////
SvgImageView::SvgImageView(QGraphicsItem *parent)
    : AbstractImageView(parent)
    , mSvgItem(new QGraphicsSvgItem(this))
{
    // At certain scales, the SVG can render outside its own bounds up to 1 pixel
    // This clips it so it isn't drawn outside the background or over the selection rect
    mSvgItem->setFlag(ItemClipsToShape);

    // So we aren't unnecessarily drawing the background for every paint()
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

void SvgImageView::loadFromDocument()
{
    Document::Ptr doc = document();
    GV_RETURN_IF_FAIL(doc);

    if (doc->loadingState() == Document::Loaded) {
        QMetaObject::invokeMethod(this, &SvgImageView::finishLoadFromDocument, Qt::QueuedConnection);
    }

    // Ensure finishLoadFromDocument is also called when
    // - loadFromDocument was called before the document was fully loaded
    // - reloading is triggered (e.g. via F5)
    connect(doc.data(), &Document::loaded, this, &SvgImageView::finishLoadFromDocument);
}

void SvgImageView::finishLoadFromDocument()
{
    QSvgRenderer *renderer = document()->svgRenderer();
    GV_RETURN_IF_FAIL(renderer);
    mSvgItem->setSharedRenderer(renderer);
    if (zoomToFit()) {
        setZoom(computeZoomToFit(), QPointF(-1, -1), ForceUpdate);
    } else if (zoomToFill()) {
        setZoom(computeZoomToFill(), QPointF(-1, -1), ForceUpdate);
    } else {
        mSvgItem->setScale(zoom());
    }
    applyPendingScrollPos();
    Q_EMIT completed();
    backgroundItem()->setVisible(true);
}

void SvgImageView::onZoomChanged()
{
    mSvgItem->setScale(zoom());
    adjustItemPos();
}

void SvgImageView::onImageOffsetChanged()
{
    adjustItemPos();
}

void SvgImageView::onScrollPosChanged(const QPointF & /* oldPos */)
{
    adjustItemPos();
}

void SvgImageView::adjustItemPos()
{
    mSvgItem->setPos((imageOffset() - scrollPos()).toPoint());
    update();
}

//// SvgViewAdapter ////
struct SvgViewAdapterPrivate {
    SvgImageView *mView;
};

SvgViewAdapter::SvgViewAdapter()
    : d(new SvgViewAdapterPrivate)
{
    d->mView = new SvgImageView;
    setWidget(d->mView);
    connect(d->mView, &SvgImageView::zoomChanged, this, &SvgViewAdapter::zoomChanged);
    connect(d->mView, &SvgImageView::zoomToFitChanged, this, &SvgViewAdapter::zoomToFitChanged);
    connect(d->mView, &SvgImageView::zoomToFillChanged, this, &SvgViewAdapter::zoomToFillChanged);
    connect(d->mView, &SvgImageView::zoomInRequested, this, &SvgViewAdapter::zoomInRequested);
    connect(d->mView, &SvgImageView::zoomOutRequested, this, &SvgViewAdapter::zoomOutRequested);
    connect(d->mView, &SvgImageView::scrollPosChanged, this, &SvgViewAdapter::scrollPosChanged);
    connect(d->mView, &SvgImageView::completed, this, &SvgViewAdapter::completed);
    connect(d->mView, &SvgImageView::previousImageRequested, this, &SvgViewAdapter::previousImageRequested);
    connect(d->mView, &SvgImageView::nextImageRequested, this, &SvgViewAdapter::nextImageRequested);
    connect(d->mView, &SvgImageView::toggleFullScreenRequested, this, &SvgViewAdapter::toggleFullScreenRequested);
}

SvgViewAdapter::~SvgViewAdapter()
{
    delete d;
}

QCursor SvgViewAdapter::cursor() const
{
    return widget()->cursor();
}

void SvgViewAdapter::setCursor(const QCursor &cursor)
{
    widget()->setCursor(cursor);
}

void SvgViewAdapter::setDocument(const Document::Ptr &doc)
{
    d->mView->setDocument(doc);
}

Document::Ptr SvgViewAdapter::document() const
{
    return d->mView->document();
}

void SvgViewAdapter::loadConfig()
{
    d->mView->backgroundItem()->setMode(GwenviewConfig::alphaBackgroundMode());
    d->mView->backgroundItem()->setColor(GwenviewConfig::alphaBackgroundColor());
    d->mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
}

void SvgViewAdapter::setZoomToFit(bool on)
{
    d->mView->setZoomToFit(on);
}

void SvgViewAdapter::setZoomToFill(bool on, const QPointF &center)
{
    d->mView->setZoomToFill(on, center);
}

bool SvgViewAdapter::zoomToFit() const
{
    return d->mView->zoomToFit();
}

bool SvgViewAdapter::zoomToFill() const
{
    return d->mView->zoomToFill();
}

qreal SvgViewAdapter::zoom() const
{
    return d->mView->zoom();
}

void SvgViewAdapter::setZoom(qreal zoom, const QPointF &center)
{
    d->mView->setZoom(zoom, center);
}

qreal SvgViewAdapter::computeZoomToFit() const
{
    return d->mView->computeZoomToFit();
}

qreal SvgViewAdapter::computeZoomToFill() const
{
    return d->mView->computeZoomToFill();
}

QPointF SvgViewAdapter::scrollPos() const
{
    return d->mView->scrollPos();
}

void SvgViewAdapter::setScrollPos(const QPointF &pos)
{
    d->mView->setScrollPos(pos);
}

QRectF SvgViewAdapter::visibleDocumentRect() const
{
    return QRectF(d->mView->imageOffset(), d->mView->visibleImageSize());
}

AbstractImageView *SvgViewAdapter::imageView() const
{
    return d->mView;
}

} // namespace
