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

static const int UNIT_STEP = 16;

struct AbstractImageViewPrivate {
	enum Verbosity {
		Silent,
		Notify
	};
	AbstractImageView* q;
	Document::Ptr mDocument;
	qreal mZoom;
	bool mZoomToFit;
	QPointF mImageOffset;
	QPointF mScrollPos;
	QPointF mLastDragPos;

	void adjustImageOffset(Verbosity verbosity = Notify) {
		QSizeF zoomedDocSize = q->documentSize() * mZoom;
		QSizeF viewSize = q->boundingRect().size();
		QPointF offset(
			qMax((viewSize.width() - zoomedDocSize.width()) / 2, 0.),
			qMax((viewSize.height() - zoomedDocSize.height()) / 2, 0.)
			);
		if (offset != mImageOffset) {
			mImageOffset = offset;
			if (verbosity == Notify) {
				q->onImageOffsetChanged();
			}
		}
	}

	void adjustScrollPos(Verbosity verbosity = Notify) {
		setScrollPos(mScrollPos, verbosity);
	}

	void setScrollPos(const QPointF& _newPos, Verbosity verbosity = Notify) {
		QSizeF zoomedDocSize = q->documentSize() * mZoom;
		QSizeF viewSize = q->boundingRect().size();
		QPointF newPos(
			qBound(0., _newPos.x(), zoomedDocSize.width() - viewSize.width()),
			qBound(0., _newPos.y(), zoomedDocSize.height() - viewSize.height())
			);
		if (newPos != mScrollPos) {
			QPointF oldPos = mScrollPos;
			mScrollPos = newPos;
			if (verbosity == Notify) {
				q->onScrollPosChanged(oldPos);
			}
		}
	}
};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate) {
	d->q = this;
	d->mZoom = 1;
	d->mZoomToFit = true;
	d->mImageOffset = QPointF(0, 0);
	d->mScrollPos = QPointF(0, 0);
	setCursor(Qt::OpenHandCursor);
	setFocusPolicy(Qt::WheelFocus);
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

void AbstractImageView::setZoom(qreal zoom, const QPointF& _center) {
	qreal oldZoom = d->mZoom;
	if (qFuzzyCompare(zoom, oldZoom)) {
		return;
	}
	d->mZoom = zoom;

	QPointF center;
	if (_center == QPointF(-1, -1)) {
		center = boundingRect().center();
	} else {
		center = _center;
	}

	/*
	We want to keep the point at viewport coordinates "center" at the same
	position after zooming. The coordinates of this point in image coordinates
	can be expressed like this:

	                      oldScroll + center
	imagePointAtOldZoom = ------------------
	                           oldZoom

	                   scroll + center
	imagePointAtZoom = ---------------
	                        zoom

	So we want:

	    imagePointAtOldZoom = imagePointAtZoom

	    oldScroll + center   scroll + center
	<=> ------------------ = ---------------
	          oldZoom             zoom

	              zoom
	<=> scroll = ------- (oldScroll + center) - center
	             oldZoom
	*/

	/*
	Compute oldScroll
	It's useless to take the new offset in consideration because if a direction
	of the new offset is not 0, we won't be able to center on a specific point
	in that direction.
	*/
	QPointF oldScroll = scrollPos() - imageOffset();

	QPointF scroll = (zoom / oldZoom) * (oldScroll + center) - center;

	d->adjustImageOffset(AbstractImageViewPrivate::Silent);
	d->setScrollPos(scroll, AbstractImageViewPrivate::Silent);
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
	}
	// We do not set zoom to 1 if zoomToFit is off, this is up to the code
	// calling us. It may went to zoom to some other level and/or to zoom on
	// a particular position
	zoomToFitChanged(d->mZoomToFit);
}

void AbstractImageView::resizeEvent(QGraphicsSceneResizeEvent* event) {
    QGraphicsWidget::resizeEvent(event);
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		d->adjustImageOffset();
		d->adjustScrollPos();
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
	d->mLastDragPos = event->pos();
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseMoveEvent(event);

	QPointF mousePos = event->pos();
	QPointF newScrollPos = d->mScrollPos + d->mLastDragPos - mousePos;

	// Wrap mouse pos
	qreal maxWidth = boundingRect().width();
	qreal maxHeight = boundingRect().height();
	// We need a margin because if the window is maximized, the mouse may not
	// be able to go past the bounding rect.
	// The mouse get placed 1 pixel before/after the margin to avoid getting
	// considered as needing to wrap the other way in next mouseMoveEvent
	// (because we don't check the move vector)
	const int margin = 5;
	if (mousePos.x() <= margin) {
		mousePos.setX(maxWidth - margin - 1);
	} else if (mousePos.x() >= maxWidth - margin) {
		mousePos.setX(margin + 1);
	}
	if (mousePos.y() <= margin) {
		mousePos.setY(maxHeight - margin - 1);
	} else if (mousePos.y() >= maxHeight - margin) {
		mousePos.setY(margin + 1);
	}

	// Set mouse pos (Hackish translation to screen coords!)
	QPointF screenDelta = event->screenPos() - event->pos();
	QCursor::setPos((mousePos + screenDelta).toPoint());

	d->mLastDragPos = mousePos;
	d->setScrollPos(newScrollPos);

}

void AbstractImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseReleaseEvent(event);
	setCursor(Qt::OpenHandCursor);
}

void AbstractImageView::keyPressEvent(QKeyEvent* event) {
	QPointF delta(0, 0);
	qreal pageStep = boundingRect().height();
	qreal unitStep;
	if (event->modifiers() & Qt::ShiftModifier) {
		unitStep = pageStep / 2;
	} else {
		unitStep = UNIT_STEP;
	}
	switch (event->key()) {
	case Qt::Key_Left:
		delta.setX(-unitStep);
		break;
	case Qt::Key_Right:
		delta.setX(unitStep);
		break;
	case Qt::Key_Up:
		delta.setY(-unitStep);
		break;
	case Qt::Key_Down:
		delta.setY(unitStep);
		break;
	case Qt::Key_PageUp:
		delta.setY(-pageStep);
		break;
	case Qt::Key_PageDown:
		delta.setY(pageStep);
		break;
	case Qt::Key_Home:
		d->setScrollPos(QPointF(d->mScrollPos.x(), 0));
		return;
	case Qt::Key_End:
		d->setScrollPos(QPointF(d->mScrollPos.x(), documentSize().height() * zoom()));
		return;
	default:
		return;
	}
	d->setScrollPos(d->mScrollPos + delta);
}

QPointF AbstractImageView::imageOffset() const {
	return d->mImageOffset;
}

QPointF AbstractImageView::scrollPos() const {
	return d->mScrollPos;
}

QPointF AbstractImageView::mapToView(const QPointF& imagePos) const {
	return imagePos * d->mZoom + d->mImageOffset - d->mScrollPos;
}

QRectF AbstractImageView::mapToView(const QRectF& imageRect) const {
	return QRectF(
		mapToView(imageRect.topLeft()),
		imageRect.size() * zoom()
		);
}

QPointF AbstractImageView::mapToImage(const QPointF& viewPos) const {
	return (viewPos - d->mImageOffset + d->mScrollPos) / d->mZoom;
}

} // namespace
