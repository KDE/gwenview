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
: QGraphicsWidget(parent)
, mSvgItem(new QGraphicsSvgItem(this))
, mZoom(1)
, mZoomToFit()
{
	setCursor(Qt::OpenHandCursor);
	setFlag(ItemIsSelectable);
}

void SvgImageView::setDocument(Document::Ptr doc) {
	mDocument = doc;
	QSvgRenderer* renderer = new QSvgRenderer(this);
	renderer->load(doc->rawData());
	mSvgItem->setSharedRenderer(renderer);
	if (mZoomToFit) {
		setZoom(computeZoomToFit());
	}
}

QSizeF SvgImageView::documentSize() const {
	return QSizeF(); //mRenderer->defaultSize();
}

qreal SvgImageView::zoom() const {
	return mZoom;
}

void SvgImageView::setZoom(qreal zoom) {
	mZoom = zoom;
	mSvgItem->setScale(mZoom);
	adjustPos();
	zoomChanged(mZoom);
}

bool SvgImageView::zoomToFit() const {
	return mZoomToFit;
}

void SvgImageView::setZoomToFit(bool on) {
	mZoomToFit = on;
	if (on) {
		setZoom(computeZoomToFit());
	} else {
		setZoom(1.);
	}
	zoomToFitChanged(mZoomToFit);
}

void SvgImageView::adjustPos() {
	QSizeF visibleSize = mSvgItem->renderer()->defaultSize() * mZoom;
	QSizeF viewSize = boundingRect().size();
	QSizeF scrollRange = visibleSize - viewSize;
	QPointF p = mSvgItem->pos();
	if (viewSize.width() > visibleSize.width()) {
		p.setX((viewSize.width() - visibleSize.width()) / 2);
	} else {
		p.setX(qBound(-scrollRange.width(), p.x(), 0.));
	}
	if (viewSize.height() > visibleSize.height()) {
		p.setY((viewSize.height() - visibleSize.height()) / 2);
	} else {
		p.setY(qBound(-scrollRange.height(), p.y(), 0.));
	}
	mSvgItem->setPos(p);
}

void SvgImageView::resizeEvent(QGraphicsSceneResizeEvent* event) {
    QGraphicsWidget::resizeEvent(event);
	if (mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		adjustPos();
	}
}

qreal SvgImageView::computeZoomToFit() const {
	QSizeF docSize = mSvgItem->renderer()->defaultSize();
	QSizeF viewSize = boundingRect().size();
	qreal fitWidth = viewSize.width() / docSize.width();
	qreal fitHeight = viewSize.height() / docSize.height();
	return qMin(fitWidth, fitHeight);
}

void SvgImageView::updateBuffer(const QRegion& region) {
	/*
	if (buffer().isNull()) {
		return;
	}
	buffer().fill(Qt::transparent);
	QPainter painter(&buffer());
	QRect dstRect = region.isEmpty() ? buffer().rect() : region.boundingRect();
	QRect srcRect = QRect(
		(dstRect.topLeft() + scrollPos().toPoint()) / zoom(),
		dstRect.size() / zoom()
		);
	mRenderer->setViewBox(srcRect);
	mRenderer->render(&painter, QRectF(dstRect));
	update();
	*/
}

void SvgImageView::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mousePressEvent(event);
	setCursor(Qt::ClosedHandCursor);
	mStartDragOffset = mapToItem(mSvgItem, event->lastPos()) * mZoom;
}

void SvgImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseMoveEvent(event);
	QPointF newPos = mapToItem(this, event->lastPos()) - mStartDragOffset;
	mSvgItem->setPos(newPos);
	adjustPos();
}

void SvgImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseReleaseEvent(event);
	setCursor(Qt::OpenHandCursor);
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
	d->mView->setZoom(zoom); //, center);
}


qreal SvgViewAdapter::computeZoomToFit() const {
	return d->mView->computeZoomToFit();
}

} // namespace
