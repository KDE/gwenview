// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QGraphicsView>

// KDE
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
};


SvgViewAdapter::SvgViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new SvgViewAdapterPrivate) {
	d->mRenderer = new KSvgRenderer(this);
	d->mScene = new QGraphicsScene(this);
	d->mView = new QGraphicsView(d->mScene, parent);
	d->mView->setFrameStyle(QFrame::NoFrame);
	d->mView->setDragMode(QGraphicsView::ScrollHandDrag);
	d->mItem = 0;

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

	if (!d->mRenderer->load(d->mDocument->rawData())) {
		return;
	}
	delete d->mItem;
	d->mItem = new QGraphicsSvgItem();
	d->mItem->setSharedRenderer(d->mRenderer);
	d->mScene->addItem(d->mItem);
}


Document::Ptr SvgViewAdapter::document() const {
	return d->mDocument;
}


void SvgViewAdapter::setZoomToFit(bool) {
}


bool SvgViewAdapter::zoomToFit() const {
	return false;
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


} // namespace
