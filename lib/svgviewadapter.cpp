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
#include <kurl.h>

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

	if (!d->mRenderer->load(d->mDocument->url().path())) {
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


} // namespace
