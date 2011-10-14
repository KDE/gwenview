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

class SvgWidget : public QGraphicsWidget {
public:
	SvgWidget(QGraphicsItem* parent = 0)
	: QGraphicsWidget(parent)
	, mRenderer(new QSvgRenderer(this))
	{}

	void loadFromDocument(Document::Ptr doc) {
		mRenderer->load(doc->rawData());
		updateCache();
	}

protected:
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) {
		painter->drawPixmap(0, 0, mCachePix);
	}

private:
	QSvgRenderer* mRenderer;
	QPixmap mCachePix;

	void updateCache() {
		mCachePix = QPixmap(boundingRect().size().toSize());
		mCachePix.fill(Qt::transparent);
		QPainter painter(&mCachePix);
		mRenderer->render(&painter, boundingRect());
		update();
	}
};

struct SvgViewAdapterPrivate {
	Document::Ptr mDocument;
	SvgWidget* mWidget;
	bool mZoomToFit;
};


SvgViewAdapter::SvgViewAdapter()
: d(new SvgViewAdapterPrivate) {
	d->mZoomToFit = true;
	d->mWidget = new SvgWidget;
	setWidget(d->mWidget);
}


void SvgViewAdapter::installEventFilterOnViewWidgets(QObject* /*object*/) {
	// FIXME: QGV
	//d->mView->viewport()->installEventFilter(object);
	// Necessary to receive key{Press,Release} events
	//d->mView->installEventFilter(object);
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
	d->mWidget->loadFromDocument(d->mDocument);
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
	return 1;
	// FIXME: QGV
	//return d->mView->matrix().m11();
}


void SvgViewAdapter::setZoom(qreal /*zoom*/, const QPoint& /*center*/) {
	// FIXME: QGV
	/*
	QMatrix matrix;
	matrix.scale(zoom, zoom);
	d->mView->setMatrix(matrix);
	emit zoomChanged(zoom);
	*/
}


qreal SvgViewAdapter::computeZoomToFit() const {
	return qMin(computeZoomToFitWidth(), computeZoomToFitHeight());
}


qreal SvgViewAdapter::computeZoomToFitWidth() const {
	return 1;
	// FIXME: QGV
	/*
	int width = d->mScene->width();
	return width != 0 ? (qreal(d->mView->viewport()->width()) / width) : 1;
	*/
}


qreal SvgViewAdapter::computeZoomToFitHeight() const {
	return 1;
	// FIXME: QGV
	/*
	int height = d->mScene->height();
	return height != 0 ? (qreal(d->mView->viewport()->height()) / height) : 1;
	*/
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
