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
#include <QGraphicsSvgItem>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>
#include <QSvgRenderer>

// KDE
#include <KDebug>

// Local
#include "document/documentfactory.h"
#include <qgraphicssceneevent.h>
#include <lib/gvdebug.h>

namespace Gwenview
{

/// SvgImageView ////
SvgImageView::SvgImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, mSvgItem(new QGraphicsSvgItem(this))
{
}

void SvgImageView::loadFromDocument()
{
    Document::Ptr doc = document();
    GV_RETURN_IF_FAIL(doc);

    if (doc->loadingState() < Document::Loaded) {
        connect(doc.data(), SIGNAL(loaded(QUrl)),
            SLOT(finishLoadFromDocument()));
    } else {
        QMetaObject::invokeMethod(this, "finishLoadFromDocument", Qt::QueuedConnection);
    }
}

void SvgImageView::finishLoadFromDocument()
{
    QSvgRenderer* renderer = document()->svgRenderer();
    GV_RETURN_IF_FAIL(renderer);
    mSvgItem->setSharedRenderer(renderer);
    if (zoomToFit()) {
        setZoom(computeZoomToFit(), QPointF(-1, -1), ForceUpdate);
    } else {
        mSvgItem->setScale(zoom());
    }
    applyPendingScrollPos();
    completed();
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

void SvgImageView::onScrollPosChanged(const QPointF& /* oldPos */)
{
    adjustItemPos();
}

void SvgImageView::adjustItemPos()
{
    mSvgItem->setPos(imageOffset() - scrollPos());
}

//// SvgViewAdapter ////
struct SvgViewAdapterPrivate
{
    SvgImageView* mView;
};

SvgViewAdapter::SvgViewAdapter()
: d(new SvgViewAdapterPrivate)
{
    d->mView = new SvgImageView;
    setWidget(d->mView);
    connect(d->mView, &SvgImageView::zoomChanged, this, &SvgViewAdapter::zoomChanged);
    connect(d->mView, &SvgImageView::zoomToFitChanged, this, &SvgViewAdapter::zoomToFitChanged);
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

void SvgViewAdapter::setCursor(const QCursor& cursor)
{
    widget()->setCursor(cursor);
}

void SvgViewAdapter::setDocument(Document::Ptr doc)
{
    d->mView->setDocument(doc);
}

Document::Ptr SvgViewAdapter::document() const
{
    return d->mView->document();
}

void SvgViewAdapter::setZoomToFit(bool on)
{
    d->mView->setZoomToFit(on);
}

bool SvgViewAdapter::zoomToFit() const
{
    return d->mView->zoomToFit();
}

qreal SvgViewAdapter::zoom() const
{
    return d->mView->zoom();
}

void SvgViewAdapter::setZoom(qreal zoom, const QPointF& center)
{
    d->mView->setZoom(zoom, center);
}

qreal SvgViewAdapter::computeZoomToFit() const
{
    return d->mView->computeZoomToFit();
}

QPointF SvgViewAdapter::scrollPos() const
{
    return d->mView->scrollPos();
}

void SvgViewAdapter::setScrollPos(const QPointF& pos)
{
    d->mView->setScrollPos(pos);
}

} // namespace
