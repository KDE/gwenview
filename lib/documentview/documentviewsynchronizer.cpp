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


struct DocumentViewSynchronizerPrivate {
	DocumentView* mView1;
	DocumentView* mView2;
	QList<PropertyBinder*> mPropertyBinders;

	void addBinder(const char* name) {
		PropertyBinder* binder = new PropertyBinder;
		binder->bind(mView1, name, mView2, name);
		mView2->setProperty(name, mView1->property(name));
		mPropertyBinders << binder;
	}

	void deletePropertyBinders() {
		qDeleteAll(mPropertyBinders);
		mPropertyBinders.clear();
	}
};


DocumentViewSynchronizer::DocumentViewSynchronizer(DocumentView* view1, DocumentView* view2, QObject* parent)
: QObject(parent)
, d(new DocumentViewSynchronizerPrivate) {
	d->mView1 = view1;
	d->mView2 = view2;
}


DocumentViewSynchronizer::~DocumentViewSynchronizer() {
	d->deletePropertyBinders();
	delete d;
}


void DocumentViewSynchronizer::setActive(bool active) {
	if (!active) {
		d->deletePropertyBinders();
		return;
	}
	d->addBinder("zoomToFit");
	d->addBinder("zoom");
}


} // namespace
