// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include "abstractimageview.moc"

// Local

// KDE
#include <kdebug.h>
#include <kurl.h>

// Qt
#include <QCursor>
#include <QGraphicsSceneMouseEvent>

namespace Gwenview {

static const int KEY_SCROLL_STEP = 16;

struct AbstractImageViewPrivate {
	AbstractImageView* q;
	Document::Ptr mDocument;
	qreal mZoom;
	bool mZoomToFit;
	QPointF mStartDragOffset;
	QPointF mImagePos;

	void setImagePos(const QPointF& pos) {
		QPointF oldPos = mImagePos;
		mImagePos = clippedPos(pos);
		if (oldPos != mImagePos) {
			q->onImagePosChanged(oldPos);
		}
	}

	void clipImagePos() {
		// This will not change anything *unless* the clipping bounds changed
		setImagePos(mImagePos);
	}

	QPointF clippedPos(const QPointF& _pos) const {
		QPointF pos = _pos;
		QSizeF visibleSize = q->documentSize() * mZoom;
		QSizeF viewSize = q->boundingRect().size();
		QSizeF scrollRange = visibleSize - viewSize;
		if (scrollRange.width() < 0) {
			pos.setX(-scrollRange.width() / 2);
		} else {
			pos.setX(qBound(-scrollRange.width(), pos.x(), 0.));
		}
		if (scrollRange.height() < 0) {
			pos.setY(-scrollRange.height() / 2);
		} else {
			pos.setY(qBound(-scrollRange.height(), pos.y(), 0.));
		}
		return pos;
	}

};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate) {
	d->q = this;
	d->mZoom = 1;
	d->mZoomToFit = true;
	setCursor(Qt::OpenHandCursor);
	setFlag(ItemIsFocusable);
	setFlag(ItemIsSelectable);
}

AbstractImageView::~AbstractImageView() {
	delete d;
}

Document::Ptr AbstractImageView::document() const {
	return d->mDocument;
}

void AbstractImageView::setDocument(Document::Ptr doc) {
	d->mDocument = doc;
	loadFromDocument();
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
	}
}

QSizeF AbstractImageView::documentSize() const {
	return d->mDocument ? d->mDocument->size() : QSizeF();
}

qreal AbstractImageView::zoom() const {
	return d->mZoom;
}

void AbstractImageView::setZoom(qreal zoom, const QPointF& /*center*/) {
	d->mZoom = zoom;
	d->clipImagePos();
	onZoomChanged();
	zoomChanged(d->mZoom);
}

bool AbstractImageView::zoomToFit() const {
	return d->mZoomToFit;
}

void AbstractImageView::setZoomToFit(bool on) {
	d->mZoomToFit = on;
	if (on) {
		setZoom(computeZoomToFit());
	} else {
		setZoom(1.);
	}
	zoomToFitChanged(d->mZoomToFit);
}

void AbstractImageView::resizeEvent(QGraphicsSceneResizeEvent* event) {
    QGraphicsWidget::resizeEvent(event);
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		d->clipImagePos();
	}
}

qreal AbstractImageView::computeZoomToFit() const {
	QSizeF docSize = documentSize();
	if (docSize.isEmpty()) {
		return 1;
	}
	QSizeF viewSize = boundingRect().size();
	qreal fitWidth = viewSize.width() / docSize.width();
	qreal fitHeight = viewSize.height() / docSize.height();
	return qMin(fitWidth, fitHeight);
}

void AbstractImageView::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mousePressEvent(event);
	setCursor(Qt::ClosedHandCursor);
	d->mStartDragOffset = event->lastPos() - d->mImagePos;
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseMoveEvent(event);
	QPointF newPos = event->lastPos() - d->mStartDragOffset;
	d->setImagePos(newPos);
}

void AbstractImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseReleaseEvent(event);
	setCursor(Qt::OpenHandCursor);
}

void AbstractImageView::keyPressEvent(QKeyEvent* event) {
	QPointF delta(0, 0);
	switch (event->key()) {
	case Qt::Key_Left:
		delta.setX(-1);
		break;
	case Qt::Key_Right:
		delta.setX(1);
		break;
	case Qt::Key_Up:
		delta.setY(-1);
		break;
	case Qt::Key_Down:
		delta.setY(1);
		break;
	default:
		return;
	}
	delta *= KEY_SCROLL_STEP;
	d->setImagePos(d->mImagePos + delta);
}

void AbstractImageView::keyReleaseEvent(QKeyEvent* /*event*/) {
}

QPointF AbstractImageView::imagePos() const {
	return d->mImagePos;
}

} // namespace
