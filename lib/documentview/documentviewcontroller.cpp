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
#include "documentviewcontroller.moc"

// Local
#include "abstractdocumentviewadapter.h"
#include "documentview.h"
#include <lib/zoomwidget.h>

// KDE

// Qt

namespace Gwenview {


struct DocumentViewControllerPrivate {
	DocumentViewController* q;
	KActionCollection* mActionCollection;
	DocumentView* mView;
	ZoomWidget* mZoomWidget;

	void setupActions() {
	}

	void connectZoomWidget() {
		if (!mZoomWidget || !mView) {
			return;
		}

		QObject::connect(mZoomWidget, SIGNAL(zoomChanged(qreal)),
			mView, SLOT(setZoom(qreal)));

		QObject::connect(mView, SIGNAL(minimumZoomChanged(qreal)),
			mZoomWidget, SLOT(setMinimumZoom(qreal)));
		QObject::connect(mView, SIGNAL(zoomChanged(qreal)),
			mZoomWidget, SLOT(setZoom(qreal)));
	}

	void connectView() {
		QObject::connect(mView, SIGNAL(adapterChanged()),
			q, SLOT(updateZoomWidgetVisibility()));
	}
};


DocumentViewController::DocumentViewController(KActionCollection* actionCollection, QObject* parent)
: QObject(parent)
, d(new DocumentViewControllerPrivate) {
	d->q = this;
	d->mActionCollection = actionCollection;
	d->mView = 0;
	d->mZoomWidget = 0;

	d->setupActions();
}


DocumentViewController::~DocumentViewController() {
	delete d;
}


void DocumentViewController::setView(DocumentView* view) {
	d->mView = view;
	d->connectView();
	d->connectZoomWidget();
	updateZoomWidgetVisibility();
}


void DocumentViewController::setZoomWidget(ZoomWidget* widget) {
	d->mZoomWidget = widget;

	#define getAction(x) d->mActionCollection->action(x)
	d->mZoomWidget->setActions(
		getAction("view_zoom_to_fit"),
		getAction("view_actual_size"),
		getAction("view_zoom_in"),
		getAction("view_zoom_out"));
	#undef getAction

	d->mZoomWidget->setMaximumZoom(qreal(DocumentView::MaximumZoom));

	d->connectZoomWidget();
	updateZoomWidgetVisibility();
}


ZoomWidget* DocumentViewController::zoomWidget() const {
	return d->mZoomWidget;
}


void DocumentViewController::updateZoomWidgetVisibility() {
	if (!d->mZoomWidget) {
		return;
	}
	d->mZoomWidget->setVisible(d->mView && d->mView->adapter()->canZoom());
}


} // namespace
