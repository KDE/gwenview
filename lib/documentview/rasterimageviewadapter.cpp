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
#include "rasterimageviewadapter.h"

// Local
#include <lib/document/documentfactory.h>
#include <lib/documentview/rasterimageview.h>
#include <lib/gwenviewconfig.h>

// KDE
#include <QUrl>

// Qt

namespace Gwenview
{

//// RasterImageViewAdapter ////
struct RasterImageViewAdapterPrivate
{
    RasterImageViewAdapter* q;
    RasterImageView* mView;
};

RasterImageViewAdapter::RasterImageViewAdapter()
: d(new RasterImageViewAdapterPrivate)
{
    d->q = this;
    d->mView = new RasterImageView;
    connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)));
    connect(d->mView, SIGNAL(zoomToFitChanged(bool)), SIGNAL(zoomToFitChanged(bool)));
    connect(d->mView, SIGNAL(zoomInRequested(QPointF)), SIGNAL(zoomInRequested(QPointF)));
    connect(d->mView, SIGNAL(zoomOutRequested(QPointF)), SIGNAL(zoomOutRequested(QPointF)));
    connect(d->mView, SIGNAL(scrollPosChanged()), SIGNAL(scrollPosChanged()));
    connect(d->mView, SIGNAL(completed()), SIGNAL(completed()));
    connect(d->mView, SIGNAL(previousImageRequested()), SIGNAL(previousImageRequested()));
    connect(d->mView, SIGNAL(nextImageRequested()), SIGNAL(nextImageRequested()));
    connect(d->mView, SIGNAL(toggleFullScreenRequested()), SIGNAL(toggleFullScreenRequested()));
    setWidget(d->mView);
}

RasterImageViewAdapter::~RasterImageViewAdapter()
{
    delete d;
}

QCursor RasterImageViewAdapter::cursor() const
{
    return d->mView->cursor();
}

void RasterImageViewAdapter::setCursor(const QCursor& cursor)
{
    d->mView->setCursor(cursor);
}

void RasterImageViewAdapter::setDocument(Document::Ptr doc)
{
    d->mView->setDocument(doc);

    connect(doc.data(), SIGNAL(loadingFailed(QUrl)), SLOT(slotLoadingFailed()));
    if (doc->loadingState() == Document::LoadingFailed) {
        slotLoadingFailed();
    }
}

qreal RasterImageViewAdapter::zoom() const
{
    return d->mView->zoom();
}

void RasterImageViewAdapter::setZoomToFit(bool on)
{
    d->mView->setZoomToFit(on);
}

bool RasterImageViewAdapter::zoomToFit() const
{
    return d->mView->zoomToFit();
}

void RasterImageViewAdapter::setZoom(qreal zoom, const QPointF& center)
{
    d->mView->setZoom(zoom, center);
}

qreal RasterImageViewAdapter::computeZoomToFit() const
{
    return d->mView->computeZoomToFit();
}

Document::Ptr RasterImageViewAdapter::document() const
{
    return d->mView->document();
}

void RasterImageViewAdapter::slotLoadingFailed()
{
    d->mView->setDocument(Document::Ptr());
}

void RasterImageViewAdapter::loadConfig()
{
    d->mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
    d->mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
    d->mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
}

RasterImageView* RasterImageViewAdapter::rasterImageView() const
{
    return d->mView;
}

QPointF RasterImageViewAdapter::scrollPos() const
{
    return d->mView->scrollPos();
}

void RasterImageViewAdapter::setScrollPos(const QPointF& pos)
{
    d->mView->setScrollPos(pos);
}

QRectF RasterImageViewAdapter::visibleDocumentRect() const
{
    return QRectF(d->mView->imageOffset(), d->mView->visibleImageSize());
}


} // namespace
