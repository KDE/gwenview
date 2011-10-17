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
#include <QGraphicsWidget>
#include <QPainter>
#include <QSvgRenderer>

// KDE
#include <kdebug.h>

// Local
#include "document/documentfactory.h"

namespace Gwenview {

/// SvgImageView ////
SvgImageView::SvgImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, mRenderer(new QSvgRenderer(this))
{}

void SvgImageView::setDocument(Document::Ptr doc) {
	AbstractImageView::setDocument(doc);
	mRenderer->load(doc->rawData());
	updateCache();
}

QSizeF SvgImageView::documentSize() const {
	return mRenderer->defaultSize();
}

void SvgImageView::updateCache() {
	mCachePix = QPixmap((documentSize() * zoom()).toSize());
	mCachePix.fill(Qt::transparent);
	QPainter painter(&mCachePix);
	mRenderer->render(&painter, QRectF(mCachePix.rect()));
	update();
}

//// SvgViewAdapter ////
struct SvgViewAdapterPrivate {
	SvgImageView* mView;
};


SvgViewAdapter::SvgViewAdapter()
: d(new SvgViewAdapterPrivate) {
	d->mView = new SvgImageView;
	setWidget(d->mView);
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
	emit zoomChanged(zoom);
}


qreal SvgViewAdapter::computeZoomToFit() const {
	return d->mView->computeZoomToFit();
}

} // namespace
