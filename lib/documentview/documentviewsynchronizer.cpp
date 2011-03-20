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
#include <propertybinder.h>

// KDE

// Qt

namespace Gwenview {


typedef QList<DocumentView*> ViewList;

struct DocumentViewSynchronizerPrivate {
	ViewList mViews;
	DocumentView* mCurrentView;
	QList<PropertyBinder*> mPropertyBinders;
	bool mActive;

	void addBinder(const char* name) {
		PropertyBinder* binder = new PropertyBinder;
		Q_FOREACH(DocumentView* view, mViews) {
			if (view == mCurrentView) {
				continue;
			}
			binder->bind(mCurrentView, name, view, name);
			view->setProperty(name, mCurrentView->property(name));
		}
		mPropertyBinders << binder;
	}

	void deleteBinders() {
		qDeleteAll(mPropertyBinders);
		mPropertyBinders.clear();
	}

	void updateBinders() {
		deleteBinders();
		if (!mCurrentView || !mActive) {
			return;
		}
		addBinder("zoomToFit");
		addBinder("zoom");
		addBinder("position");
	}
};


DocumentViewSynchronizer::DocumentViewSynchronizer(QObject* parent)
: QObject(parent)
, d(new DocumentViewSynchronizerPrivate) {
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
	d->updateBinders();
}

void DocumentViewSynchronizer::setActive(bool active) {
	d->mActive = active;
	d->updateBinders();
}


} // namespace
