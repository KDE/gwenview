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
#if 0

struct AbstractImageViewPrivate {
	AbstractImageView* q;
	Document::Ptr mDoc;
	qreal mZoom;
	bool mZoomToFit;
	QPixmap mCurrentBuffer;
	// The alternate buffer is useful when scrolling: existing content is copied
	// to mAlternateBuffer and buffers are then swapped. This avoids the
	// allocation of a new QPixmap everytime the view is scrolled.
	QPixmap mAlternateBuffer;
	QTimer* mZoomToFitUpdateTimer;
	QPointF mScrollPos;

	void setupZoomToFitUpdateTimer() {
		mZoomToFitUpdateTimer = new QTimer(q);
		mZoomToFitUpdateTimer->setInterval(500);
		mZoomToFitUpdateTimer->setSingleShot(true);
		QObject::connect(mZoomToFitUpdateTimer, SIGNAL(timeout()),
			q, SLOT(updateZoomToFit()));
	}

	QPointF boundedScrollPos(const QPointF& pos) const {
		QSizeF maxScroll = q->documentSize() * mZoom - q->size();
		qreal x = qBound(0., pos.x(), maxScroll.width());
		qreal y = qBound(0., pos.y(), maxScroll.height());
		return QPointF(x, y);
	}

	void adjustScrollPos() {
		q->setScrollPos(mScrollPos);
	}
};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate) {
	d->q = this;
	d->mZoom = 1;
	d->mZoomToFit = true;
	d->mScrollPos = QPointF(0, 0);
	setFlag(ItemIsFocusable);
	setFlag(ItemIsSelectable);

	d->setupZoomToFitUpdateTimer();
}

AbstractImageView::~AbstractImageView() {
	delete d;
}

void AbstractImageView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) {
	QSize bufferSize = d->mCurrentBuffer.size();

	QSizeF paintSize;
	if (d->mZoomToFit) {
		paintSize = documentSize() * computeZoomToFit();
	} else {
		paintSize = bufferSize;
	}
	painter->drawPixmap(
		(boundingRect().width() - paintSize.width()) / 2,
		(boundingRect().height() - paintSize.height()) / 2,
		paintSize.width(),
		paintSize.height(),
		d->mCurrentBuffer);
}

qreal AbstractImageView::zoom() const {
	return d->mZoom;
}

void AbstractImageView::setZoom(qreal zoom, const QPointF& /*center*/) {
	d->mZoom = zoom;
	createBuffer();
	d->adjustScrollPos();
	updateBuffer();
	zoomChanged(d->mZoom);
}

bool AbstractImageView::zoomToFit() const {
	return d->mZoomToFit;
}

void AbstractImageView::setZoomToFit(bool value) {
	if (d->mZoomToFit == value) {
		return;
	}
	d->mZoomToFit = value;
	if (d->mZoomToFit) {
		setZoom(computeZoomToFit());
	}
	zoomToFitChanged(value);
}

Document::Ptr AbstractImageView::document() const {
	return d->mDoc;
}

void AbstractImageView::setDocument(Document::Ptr doc) {
	d->mDoc = doc;
}

QSizeF AbstractImageView::documentSize() const {
	return d->mDoc ? d->mDoc->size() : QSizeF();
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

void AbstractImageView::resizeEvent(QGraphicsSceneResizeEvent* event) {
    QGraphicsWidget::resizeEvent(event);
	if (d->mZoomToFit) {
		d->mZoomToFitUpdateTimer->start();
	}
	d->adjustScrollPos();
}

void AbstractImageView::updateZoomToFit() {
	if (!d->mZoomToFit) {
		return;
	}
	setZoom(computeZoomToFit());
}

QSizeF AbstractImageView::visibleImageSize() const {
	if (!d->mDoc) {
		return QSizeF();
	}
	qreal zoom;
	if (d->mZoomToFit) {
		zoom = computeZoomToFit();
	} else {
		zoom = d->mZoom;
	}

	QSizeF size = documentSize() * zoom;
	size = size.boundedTo(boundingRect().size());

	return size;
}

QRectF AbstractImageView::mapViewportToZoomedImage(const QRectF& viewportRect) const {
	// FIXME: QGV
	//QPointF offset = QPointF(0, 0); //mView->imageOffset();
	QRectF rect = QRectF(
		viewportRect.x(), //+ hScroll() - offset.x(),
		viewportRect.y(), //+ vScroll() - offset.y(),
		viewportRect.width(),
		viewportRect.height()
	);

	return rect;
}

void AbstractImageView::createBuffer() {
	QSize size = visibleImageSize().toSize();
	if (size == d->mCurrentBuffer.size()) {
		return;
	}
	if (!size.isValid()) {
		d->mAlternateBuffer = QPixmap();
		d->mCurrentBuffer = QPixmap();
		return;
	}

	d->mAlternateBuffer = QPixmap(size);
	d->mAlternateBuffer.fill(Qt::transparent);
	{
		QPainter painter(&d->mAlternateBuffer);
		painter.drawPixmap(0, 0, d->mCurrentBuffer);
	}
	qSwap(d->mAlternateBuffer, d->mCurrentBuffer);

	d->mAlternateBuffer = QPixmap();
}

QPixmap& AbstractImageView::buffer()
{
	return d->mCurrentBuffer;
}

void AbstractImageView::keyPressEvent(QKeyEvent* event) {
	kDebug() << d->mDoc->url();
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
	setScrollPos(scrollPos() + delta);
}

void AbstractImageView::keyReleaseEvent(QKeyEvent* /*event*/) {
	kDebug() << d->mDoc->url();
}

void AbstractImageView::mousePressEvent(QGraphicsSceneMouseEvent* /*event*/) {
	kDebug();
	// Necessary to get focus when clicking on a single image
	setFocus();
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* /*event*/) {
	kDebug();
}

void AbstractImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* /*event*/) {
	kDebug();
}

QPointF AbstractImageView::scrollPos() const {
	return d->mScrollPos;
}

void AbstractImageView::setScrollPos(const QPointF& _pos) {
	QPointF pos = d->boundedScrollPos(_pos);
	if (d->mScrollPos == pos) {
		return;
	}

	int dx = d->mScrollPos.x() - pos.x();
	int dy = d->mScrollPos.y() - pos.y();
	d->mScrollPos = pos;

	// FIXME: QGV
	/*
	if (d->mInsideSetZoom) {
		// Do not scroll anything: since we are zooming the whole viewport will
		// eventually be repainted
		return;
	}
	*/
	// Scroll existing
	{
		if (d->mAlternateBuffer.size() != d->mCurrentBuffer.size()) {
			d->mAlternateBuffer = QPixmap(d->mCurrentBuffer.size());
		}
		QPainter painter(&d->mAlternateBuffer);
		painter.drawPixmap(dx, dy, d->mCurrentBuffer);
	}
	qSwap(d->mCurrentBuffer, d->mAlternateBuffer);

	// Scale missing parts
	QRegion region;
	int posX = pos.x();
	int posY = pos.y();
	int width = size().width();
	int height = size().height();

	QRect rect;
	if (dx > 0) {
		rect = QRect(posX, posY, dx, height);
	} else {
		rect = QRect(posX + width + dx, posY, -dx, height);
	}
	region |= rect;

	if (dy > 0) {
		rect = QRect(posX, posY, width, dy);
	} else {
		rect = QRect(posX, posY + height + dy, width, -dy);
	}
	region |= rect;

	updateBuffer(region);
	update();
}
#endif

struct AbstractImageViewPrivate {
	AbstractImageView* q;
	QGraphicsItem* mChildItem;
	Document::Ptr mDocument;
	qreal mZoom;
	bool mZoomToFit;
	QPointF mStartDragOffset;

	void adjustPos() {
		QSizeF visibleSize = q->documentSize() * mZoom;
		QSizeF viewSize = q->boundingRect().size();
		QSizeF scrollRange = visibleSize - viewSize;
		QPointF p = mChildItem->pos();
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
		mChildItem->setPos(p);
	}

};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate) {
	d->q = this;
	d->mZoom = 1;
	d->mZoomToFit = true;
	d->mChildItem = 0;
	setCursor(Qt::OpenHandCursor);
	setFlag(ItemIsFocusable);
	setFlag(ItemIsSelectable);
}

AbstractImageView::~AbstractImageView() {
	delete d;
}

void AbstractImageView::setChildItem(QGraphicsItem* item) {
	Q_ASSERT(item);
	d->mChildItem = item;
	d->mChildItem->setParentItem(this);
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
	d->mChildItem->setScale(d->mZoom);
	d->adjustPos();
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
		d->adjustPos();
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
	d->mStartDragOffset = mapToItem(d->mChildItem, event->lastPos()) * d->mZoom;
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	QGraphicsItem::mouseMoveEvent(event);
	QPointF newPos = mapToItem(this, event->lastPos()) - d->mStartDragOffset;
	d->mChildItem->setPos(newPos);
	d->adjustPos();
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
	d->mChildItem->setPos(d->mChildItem->pos() + delta);
	d->adjustPos();
}

void AbstractImageView::keyReleaseEvent(QKeyEvent* /*event*/) {
}

} // namespace
