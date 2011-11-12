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
#include <QPainter>

namespace Gwenview {

static qreal MAX_SIZE = 128;
static qreal VIEW_OFFSET = 48;

struct BirdEyeViewPrivate {
	BirdEyeView* q;
	DocumentView* mDocView;
	QRectF mVisibleRect;

	void updateGeometry() {
		QSize size = mDocView->document()->size();
		size.scale(MAX_SIZE, MAX_SIZE, Qt::KeepAspectRatio);
		QPointF topRight = mDocView->boundingRect().topRight();
		q->setGeometry(
			QRectF(
				topRight.x() - VIEW_OFFSET - size.width(),
				topRight.y() + VIEW_OFFSET,
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
	d->q = this;
	d->mDocView = docView;
	d->updateGeometry();
	d->updateVisibleRect();
	d->updateVisibility();

	connect(docView->document().data(), SIGNAL(metaInfoUpdated()), SLOT(slotMetaInfoUpdated()));
	connect(docView, SIGNAL(zoomChanged(qreal)), SLOT(slotZoomChanged()));
	connect(docView, SIGNAL(positionChanged()), SLOT(slotPositionChanged()));
}

BirdEyeView::~BirdEyeView() {
	delete d;
}

void BirdEyeView::slotMetaInfoUpdated() {
	d->updateGeometry();
	d->updateVisibleRect();
}

void BirdEyeView::slotZoomChanged() {
	d->updateGeometry();
	d->updateVisibleRect();
	d->updateVisibility();
}

void BirdEyeView::slotPositionChanged() {
	d->updateVisibleRect();
}

void BirdEyeView::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
	painter->fillRect(boundingRect(), QColor(0, 0, 0, 64));
	painter->setPen(Qt::red);
	painter->drawRect(d->mVisibleRect.adjusted(0, 0, -1, -1));
}

} // namespace
