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
#include "imageviewadapter.moc"

// Local
#include <lib/document/documentfactory.h>
#include <lib/documentview/rasterimageview.h>
#include <lib/gwenviewconfig.h>

// KDE
#include <kurl.h>

// Qt

namespace Gwenview
{

//// ImageViewAdapter ////
struct ImageViewAdapterPrivate {
    ImageViewAdapter* q;
    RasterImageView* mView;
};

ImageViewAdapter::ImageViewAdapter()
: d(new ImageViewAdapterPrivate)
{
    d->q = this;
    d->mView = new RasterImageView;
    connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)));
    connect(d->mView, SIGNAL(zoomToFitChanged(bool)), SIGNAL(zoomToFitChanged(bool)));
    connect(d->mView, SIGNAL(zoomInRequested(QPointF)), SIGNAL(zoomInRequested(QPointF)));
    connect(d->mView, SIGNAL(zoomOutRequested(QPointF)), SIGNAL(zoomOutRequested(QPointF)));
    connect(d->mView, SIGNAL(scrollPosChanged()), SIGNAL(scrollPosChanged()));
    connect(d->mView, SIGNAL(completed()), SIGNAL(completed()));
    setWidget(d->mView);
}

ImageViewAdapter::~ImageViewAdapter()
{
    delete d;
}

QCursor ImageViewAdapter::cursor() const
{
    return d->mView->cursor();
}

void ImageViewAdapter::setCursor(const QCursor& cursor)
{
    d->mView->setCursor(cursor);
}

void ImageViewAdapter::setDocument(Document::Ptr doc)
{
    d->mView->setDocument(doc);

    connect(doc.data(), SIGNAL(loadingFailed(KUrl)), SLOT(slotLoadingFailed()));
    if (doc->loadingState() == Document::LoadingFailed) {
        slotLoadingFailed();
    }
}

qreal ImageViewAdapter::zoom() const
{
    return d->mView->zoom();
}

void ImageViewAdapter::setZoomToFit(bool on)
{
    d->mView->setZoomToFit(on);
}

bool ImageViewAdapter::zoomToFit() const
{
    return d->mView->zoomToFit();
}

void ImageViewAdapter::setZoom(qreal zoom, const QPointF& center)
{
    d->mView->setZoom(zoom, center);
}

qreal ImageViewAdapter::computeZoomToFit() const
{
    return d->mView->computeZoomToFit();
}

Document::Ptr ImageViewAdapter::document() const
{
    return d->mView->document();
}

void ImageViewAdapter::slotLoadingFailed()
{
    d->mView->setDocument(Document::Ptr());
}

void ImageViewAdapter::loadConfig()
{
    d->mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
    d->mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
    d->mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
}

RasterImageView* ImageViewAdapter::rasterImageView() const
{
    return d->mView;
}

QPointF ImageViewAdapter::scrollPos() const
{
    return d->mView->scrollPos();
}

void ImageViewAdapter::setScrollPos(const QPointF& pos)
{
    d->mView->setScrollPos(pos);
}

} // namespace
