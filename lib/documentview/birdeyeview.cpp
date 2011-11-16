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
#include "birdeyeview.moc"

// Local
#include <lib/document/document.h>
#include <lib/documentview/documentview.h>

// KDE
#include <kdebug.h>

// Qt
#include <QCursor>
#include <QGraphicsSceneEvent>
#include <QPainter>

namespace Gwenview {

static qreal MAX_SIZE = 96;
static qreal VIEW_OFFSET = MAX_SIZE / 4;
static qreal Y_POSITION_PERCENT = 1 / 3.;

struct BirdEyeViewPrivate {
	BirdEyeView* q;
	DocumentView* mDocView;
	QRectF mVisibleRect;
	QPointF mLastDragPos;

	void updateGeometry() {
		QSize size = mDocView->document()->size();
		size.scale(MAX_SIZE, MAX_SIZE, Qt::KeepAspectRatio);
		QRectF rect = mDocView->boundingRect();
		q->setGeometry(
			QRectF(
				rect.right() - VIEW_OFFSET - size.width(),
				qMax(rect.top() + rect.height() * Y_POSITION_PERCENT - size.height(), 0.),
				size.width(),
				size.height()
			));
	}

	void updateVisibleRect() {
		QSizeF docSize = mDocView->document()->size();
		qreal viewZoom = mDocView->zoom();
		qreal bevZoom = q->size().width() / docSize.width();

		mVisibleRect = QRectF(
			QPointF(mDocView->position()) / viewZoom * bevZoom,
			(mDocView->size() / viewZoom).boundedTo(docSize) * bevZoom);
		q->update();
	}

	void updateVisibility() {
		q->setVisible(mVisibleRect != q->boundingRect());
	}
};

BirdEyeView::BirdEyeView(DocumentView* docView)
: QGraphicsWidget(docView)
, d(new BirdEyeViewPrivate) {
	setFlag(ItemIsSelectable);
	d->q = this;
	d->mDocView = docView;
	d->updateGeometry();
	d->updateVisibleRect();
	d->updateVisibility();

	connect(docView->document().data(), SIGNAL(metaInfoUpdated()), SLOT(adjustGeometry()));
	connect(docView, SIGNAL(zoomChanged(qreal)), SLOT(adjustGeometry()));
	connect(docView, SIGNAL(positionChanged()), SLOT(slotPositionChanged()));
}

BirdEyeView::~BirdEyeView() {
	delete d;
}

void BirdEyeView::adjustGeometry() {
	d->updateGeometry();
	d->updateVisibleRect();
	d->updateVisibility();
}

void BirdEyeView::slotPositionChanged() {
	d->updateVisibleRect();
}

inline void drawTransparentRect(QPainter* painter, const QRectF& rect, const QColor& color) {
	QColor bg = color;
	bg.setAlphaF(.33);
	QColor fg = color;
	fg.setAlphaF(.66);
	painter->setPen(fg);
	painter->setBrush(bg);
	painter->drawRect(rect.adjusted(0, 0, -1, -1));
}

void BirdEyeView::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
	drawTransparentRect(painter, boundingRect(), Qt::black);
	drawTransparentRect(painter, d->mVisibleRect, Qt::white);
}

void BirdEyeView::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	if (d->mVisibleRect.contains(event->pos())) {
		setCursor(Qt::ClosedHandCursor);
		d->mLastDragPos = event->pos();
	}
}

void BirdEyeView::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseMoveEvent(event);
	if (d->mLastDragPos.isNull()) {
		return;
	}
	qreal ratio = d->mDocView->boundingRect().width() / boundingRect().width();

	QPointF mousePos = event->pos();
	QPointF viewPos = d->mDocView->position() + (mousePos - d->mLastDragPos) * ratio;

	d->mLastDragPos = mousePos;
	d->mDocView->setPosition(viewPos.toPoint());

}

void BirdEyeView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseReleaseEvent(event);
	if (d->mLastDragPos.isNull()) {
		return;
	}
	setCursor(Qt::OpenHandCursor);
	d->mLastDragPos = QPointF();
}

} // namespace
