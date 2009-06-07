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
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QGraphicsView>

// KDE
#include <kdebug.h>
#include <ksvgrenderer.h>

// Local
#include "document/documentfactory.h"

namespace Gwenview {


struct SvgViewAdapterPrivate {
	KSvgRenderer* mRenderer;
	QGraphicsScene* mScene;
	QGraphicsView* mView;

	Document::Ptr mDocument;
	QGraphicsSvgItem* mItem;
	bool mZoomToFit;
};


SvgViewAdapter::SvgViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new SvgViewAdapterPrivate) {
	d->mRenderer = new KSvgRenderer(this);
	d->mScene = new QGraphicsScene(this);
	d->mView = new QGraphicsView(d->mScene, parent);
	d->mView->setFrameStyle(QFrame::NoFrame);
	d->mView->setDragMode(QGraphicsView::ScrollHandDrag);
	d->mView->viewport()->installEventFilter(this);

	d->mItem = 0;
	d->mZoomToFit = true;

	setWidget(d->mView);
}


void SvgViewAdapter::installEventFilterOnViewWidgets(QObject* object) {
	d->mView->viewport()->installEventFilter(object);
}


SvgViewAdapter::~SvgViewAdapter() {
	delete d;
}


void SvgViewAdapter::setDocument(Document::Ptr doc) {
	d->mDocument = doc;
	connect(d->mDocument.data(), SIGNAL(loaded(const KUrl&)),
		SLOT(loadFromDocument()));
	loadFromDocument();
}


void SvgViewAdapter::loadFromDocument() {
	delete d->mItem;
	d->mItem = 0;

	if (!d->mRenderer->load(d->mDocument->rawData())) {
		kWarning() << "Decoding SVG failed";
		return;
	}
	d->mItem = new QGraphicsSvgItem();
	d->mItem->setSharedRenderer(d->mRenderer);
	d->mScene->addItem(d->mItem);

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
}


bool SvgViewAdapter::zoomToFit() const {
	return d->mZoomToFit;
}


qreal SvgViewAdapter::zoom() const {
	return d->mView->matrix().m11();
}


void SvgViewAdapter::setZoom(qreal zoom, const QPoint& /*center*/) {
	QMatrix matrix;
	matrix.scale(zoom, zoom);
	d->mView->setMatrix(matrix);
	emit zoomChanged(zoom);
}


qreal SvgViewAdapter::computeZoomToFit() const {
	return qMin(computeZoomToFitWidth(), computeZoomToFitHeight());
}


qreal SvgViewAdapter::computeZoomToFitWidth() const {
	int width = d->mScene->width();
	return width != 0 ? (qreal(d->mView->viewport()->width()) / width) : 1;
}


qreal SvgViewAdapter::computeZoomToFitHeight() const {
	int height = d->mScene->height();
	return height != 0 ? (qreal(d->mView->viewport()->height()) / height) : 1;
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
