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
#include <QPainter>
#include <QTimer>

namespace Gwenview {

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

	void setupZoomToFitUpdateTimer() {
		mZoomToFitUpdateTimer = new QTimer(q);
		mZoomToFitUpdateTimer->setInterval(500);
		mZoomToFitUpdateTimer->setSingleShot(true);
		QObject::connect(mZoomToFitUpdateTimer, SIGNAL(timeout()),
			q, SLOT(updateZoomToFit()));
	}
};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate) {
	d->q = this;
	d->mZoom = 1;
	d->mZoomToFit = true;
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
	updateBuffer();
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
	QPointF offset = QPointF(0, 0); //mView->imageOffset();
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
}

void AbstractImageView::keyReleaseEvent(QKeyEvent* event) {
	kDebug() << d->mDoc->url();
}

void AbstractImageView::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	kDebug();
	// Necessary to get focus when clicking on a single image
	setFocus();
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	kDebug();
}

void AbstractImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	kDebug();
}


} // namespace
