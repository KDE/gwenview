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
#include <lib/signalblocker.h>
#include <lib/zoomwidget.h>

// KDE
#include <kaction.h>
#include <kactioncategory.h>
#include <kdebug.h>
#include <klocale.h>

// Qt
#include <QAction>
#include <QEvent>

namespace Gwenview {


struct DocumentViewControllerPrivate {
	DocumentViewController* q;
	KActionCollection* mActionCollection;
	DocumentView* mView;
	ZoomWidget* mZoomWidget;

	KAction* mZoomToFitAction;
	KAction* mActualSizeAction;
	KAction* mZoomInAction;
	KAction* mZoomOutAction;
	QList<KAction*> mActions;

	void setupActions() {
		KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface","View"), mActionCollection);

		mZoomToFitAction = view->addAction("view_zoom_to_fit");
		mZoomToFitAction->setCheckable(true);
		mZoomToFitAction->setChecked(true);
		mZoomToFitAction->setText(i18n("Zoom to Fit"));
		mZoomToFitAction->setIcon(KIcon("zoom-fit-best"));
		mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));

		mActualSizeAction = view->addAction(KStandardAction::ActualSize);
		mActualSizeAction->setIcon(KIcon("zoom-original"));
		mActualSizeAction->setIconText(i18nc("@action:button Zoom to original size, shown in status bar, keep it short please", "100%"));

		mZoomInAction = view->addAction(KStandardAction::ZoomIn);
		mZoomOutAction = view->addAction(KStandardAction::ZoomOut);

		mActions << mZoomToFitAction << mActualSizeAction << mZoomInAction << mZoomOutAction;
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

		mZoomWidget->setMinimumZoom(mView->minimumZoom());
		mZoomWidget->setZoom(mView->zoom());
	}

	void updateZoomWidgetVisibility() {
		if (!mZoomWidget) {
			return;
		}
		mZoomWidget->setVisible(mView && mView->canZoom());
	}

	void updateActions() {
		const bool enabled = mView && mView->isVisible() && mView->canZoom();
		Q_FOREACH(QAction* action, mActions) {
			action->setEnabled(enabled);
		}
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
	Q_ASSERT(view);
	// Forget old view
	if (d->mView) {
		disconnect(d->mView, 0, this, 0);
		Q_FOREACH(QAction* action, d->mActions) {
			disconnect(action, 0, d->mView, 0);
		}
	}

	// Connect new view
	d->mView = view;
	connect(d->mView, SIGNAL(adapterChanged()),
		SLOT(slotAdapterChanged()));

	connect(d->mView, SIGNAL(zoomToFitChanged(bool)),
		SLOT(updateZoomToFitActionFromView()));

	connect(d->mZoomToFitAction, SIGNAL(toggled(bool)),
		d->mView, SLOT(setZoomToFit(bool)));
	connect(d->mActualSizeAction, SIGNAL(triggered()),
		d->mView, SLOT(zoomActualSize()));
	connect(d->mZoomInAction, SIGNAL(triggered()),
		d->mView, SLOT(zoomIn()));
	connect(d->mZoomOutAction, SIGNAL(triggered()),
		d->mView, SLOT(zoomOut()));

	d->updateActions();
	updateZoomToFitActionFromView();

	// Sync zoom widget
	d->connectZoomWidget();
	d->updateZoomWidgetVisibility();
}

DocumentView* DocumentViewController::view() const {
	return d->mView;
}

void DocumentViewController::setZoomWidget(ZoomWidget* widget) {
	d->mZoomWidget = widget;

	d->mZoomWidget->setActions(
		d->mZoomToFitAction,
		d->mActualSizeAction,
		d->mZoomInAction,
		d->mZoomOutAction
		);

	d->mZoomWidget->setMaximumZoom(qreal(DocumentView::MaximumZoom));

	d->connectZoomWidget();
	d->updateZoomWidgetVisibility();
}


ZoomWidget* DocumentViewController::zoomWidget() const {
	return d->mZoomWidget;
}


void DocumentViewController::slotAdapterChanged() {
	d->updateActions();
	d->updateZoomWidgetVisibility();
}


void DocumentViewController::updateZoomToFitActionFromView() {
	SignalBlocker blocker(d->mZoomToFitAction);
	d->mZoomToFitAction->setChecked(d->mView->zoomToFit());
}


} // namespace
