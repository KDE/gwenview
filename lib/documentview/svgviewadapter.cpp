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

void SvgImageView::loadFromDocument(Document::Ptr doc) {
	mRenderer->load(doc->rawData());
	updateCache();
}

QSize SvgImageView::defaultSize() const {
	return mRenderer->defaultSize();
}

void SvgImageView::updateCache() {
	mCachePix = QPixmap(defaultSize() * mZoom);
	mCachePix.fill(Qt::transparent);
	QPainter painter(&mCachePix);
	mRenderer->render(&painter, QRectF(mCachePix.rect()));
	update();
}

//// SvgViewAdapter ////
struct SvgViewAdapterPrivate {
	Document::Ptr mDocument;
	SvgImageView* mView;
	bool mZoomToFit;
};


SvgViewAdapter::SvgViewAdapter()
: d(new SvgViewAdapterPrivate) {
	d->mZoomToFit = true;
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
	d->mDocument = doc;
	connect(d->mDocument.data(), SIGNAL(loaded(KUrl)),
		SLOT(loadFromDocument()));
	loadFromDocument();
}


void SvgViewAdapter::loadFromDocument() {
	d->mView->loadFromDocument(d->mDocument);
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
	}
}


Document::Ptr SvgViewAdapter::document() const {
	return d->mDocument;
}


void SvgViewAdapter::setZoomToFit(bool on) {
	if (d->mZoomToFit == on) {
		return;
	}
	d->mZoomToFit = on;
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
	}
	zoomToFitChanged(on);
}


bool SvgViewAdapter::zoomToFit() const {
	return d->mZoomToFit;
}


qreal SvgViewAdapter::zoom() const {
	return d->mView->zoom();
}


void SvgViewAdapter::setZoom(qreal zoom, const QPointF& center) {
	d->mView->setZoom(zoom, center);
	emit zoomChanged(zoom);
}


qreal SvgViewAdapter::computeZoomToFit() const {
	return qMin(computeZoomToFitWidth(), computeZoomToFitHeight());
}


qreal SvgViewAdapter::computeZoomToFitWidth() const {
	qreal width = d->mView->defaultSize().width();
	return width != 0 ? (d->mView->size().width() / width) : 1;
}


qreal SvgViewAdapter::computeZoomToFitHeight() const {
	qreal height = d->mView->defaultSize().height();
	return height != 0 ? (d->mView->size().height() / height) : 1;
}


bool SvgViewAdapter::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::Resize) {
		if (d->mZoomToFit) {
			setZoom(computeZoomToFit());
		}
	}
	return false;
}

} // namespace
