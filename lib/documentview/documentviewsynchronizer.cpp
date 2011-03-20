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
#include "documentviewsynchronizer.moc"

// Local
#include <documentview/documentview.h>

// KDE

// Qt

namespace Gwenview {


typedef QList<DocumentView*> ViewList;

struct DocumentViewSynchronizerPrivate {
	DocumentViewSynchronizer* q;
	ViewList mViews;
	DocumentView* mCurrentView;
	bool mActive;
	QPoint mOldPosition;

	void deleteBinders() {
		Q_FOREACH(DocumentView* view, mViews) {
			QObject::disconnect(view, 0, q, 0);
		}
	}

	void updateBinders() {
		deleteBinders();
		if (!mCurrentView || !mActive) {
			return;
		}

		QObject::connect(mCurrentView, SIGNAL(zoomChanged(qreal)),
			q, SLOT(setZoom(qreal)));
		QObject::connect(mCurrentView, SIGNAL(zoomToFitChanged(bool)),
			q, SLOT(setZoomToFit(bool)));
		QObject::connect(mCurrentView, SIGNAL(positionChanged()),
			q, SLOT(updatePosition()));

		Q_FOREACH(DocumentView* view, mViews) {
			if (view == mCurrentView) {
				continue;
			}
			view->setZoom(mCurrentView->zoom());
			view->setZoomToFit(mCurrentView->zoomToFit());
		}
	}

	void updateOldPosition() {
		mOldPosition = mCurrentView->position();
	}
};


DocumentViewSynchronizer::DocumentViewSynchronizer(QObject* parent)
: QObject(parent)
, d(new DocumentViewSynchronizerPrivate) {
	d->q = this;
	d->mCurrentView = 0;
	d->mActive = false;
}


DocumentViewSynchronizer::~DocumentViewSynchronizer() {
	d->deleteBinders();
	delete d;
}


void DocumentViewSynchronizer::setDocumentViews(QList< DocumentView* > views) {
	d->mViews = views;
	d->updateBinders();
}

void DocumentViewSynchronizer::setCurrentView(DocumentView* view) {
	d->mCurrentView = view;
	d->updateOldPosition();
	d->updateBinders();
}

void DocumentViewSynchronizer::setActive(bool active) {
	d->mActive = active;
	d->updateOldPosition();
	d->updateBinders();
}

void DocumentViewSynchronizer::setZoom(qreal zoom) {
	Q_FOREACH(DocumentView* view, d->mViews) {
		if (view == d->mCurrentView) {
			continue;
		}
		view->setZoom(zoom);
	}
	d->updateOldPosition();
}

void DocumentViewSynchronizer::setZoomToFit(bool fit) {
	Q_FOREACH(DocumentView* view, d->mViews) {
		if (view == d->mCurrentView) {
			continue;
		}
		view->setZoomToFit(fit);
	}
	d->updateOldPosition();
}

void DocumentViewSynchronizer::updatePosition() {
	QPoint pos = d->mCurrentView->position();
	QPoint delta = pos - d->mOldPosition;
	d->mOldPosition = pos;
	Q_FOREACH(DocumentView* view, d->mViews) {
		if (view == d->mCurrentView) {
			continue;
		}
		view->setPosition(view->position() + delta);
	}
}

} // namespace
