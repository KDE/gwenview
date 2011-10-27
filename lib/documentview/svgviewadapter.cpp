// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include "svgviewadapter.moc"

// Qt
#include <QCursor>
#include <QEvent>
#include <QGraphicsSvgItem>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>
#include <QPainter>
#include <QSvgRenderer>

// KDE
#include <kdebug.h>

// Local
#include "document/documentfactory.h"
#include <qgraphicssceneevent.h>

namespace Gwenview {

/// SvgImageView ////
SvgImageView::SvgImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, mSvgItem(new QGraphicsSvgItem(this))
{
}

void SvgImageView::loadFromDocument() {
	QSvgRenderer* renderer = new QSvgRenderer(this);
	renderer->load(document()->rawData());
	mSvgItem->setSharedRenderer(renderer);
}

QSizeF SvgImageView::documentSize() const {
	return mSvgItem->renderer()->defaultSize();
}

void SvgImageView::onZoomChanged() {
	mSvgItem->setScale(zoom());
	adjustItemPos();
}

void SvgImageView::onImageOffsetChanged() {
	adjustItemPos();
}

void SvgImageView::onScrollPosChanged(const QPointF& /* oldPos */) {
	adjustItemPos();
}

void SvgImageView::adjustItemPos() {
	mSvgItem->setPos(imageOffset() - scrollPos());
}

//// SvgViewAdapter ////
struct SvgViewAdapterPrivate {
	SvgImageView* mView;
};


SvgViewAdapter::SvgViewAdapter()
: d(new SvgViewAdapterPrivate) {
	d->mView = new SvgImageView;
	setWidget(d->mView);
	connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)) );
	connect(d->mView, SIGNAL(zoomToFitChanged(bool)), SIGNAL(zoomToFitChanged(bool)) );
}


SvgViewAdapter::~SvgViewAdapter() {
	delete d;
}


QCursor SvgViewAdapter::cursor() const {
	return widget()->cursor();
}


void SvgViewAdapter::setCursor(const QCursor& cursor) {
	widget()->setCursor(cursor);
}


void SvgViewAdapter::setDocument(Document::Ptr doc) {
	d->mView->setDocument(doc);
}


Document::Ptr SvgViewAdapter::document() const {
	return d->mView->document();
}


void SvgViewAdapter::setZoomToFit(bool on) {
	d->mView->setZoomToFit(on);
}


bool SvgViewAdapter::zoomToFit() const {
	return d->mView->zoomToFit();
}


qreal SvgViewAdapter::zoom() const {
	return d->mView->zoom();
}


void SvgViewAdapter::setZoom(qreal zoom, const QPointF& center) {
	d->mView->setZoom(zoom, center);
}


qreal SvgViewAdapter::computeZoomToFit() const {
	return d->mView->computeZoomToFit();
}

} // namespace
