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
#include "emptyviewadapter.h"

// Qt
#include <QLabel>

// KDE
#include <klocale.h>

// Local
#include <lib/document/document.h>

namespace Gwenview {


struct EmptyViewAdapterPrivate {
	QLabel* mNoDocumentLabel;
};


EmptyViewAdapter::EmptyViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new EmptyViewAdapterPrivate) {
	d->mNoDocumentLabel = new QLabel(parent);
	d->mNoDocumentLabel->setText(i18n("No document selected"));
	d->mNoDocumentLabel->setAlignment(Qt::AlignCenter);
	d->mNoDocumentLabel->setAutoFillBackground(true);
	d->mNoDocumentLabel->setBackgroundRole(QPalette::Base);
	d->mNoDocumentLabel->setForegroundRole(QPalette::Text);

	setWidget(d->mNoDocumentLabel);
}


EmptyViewAdapter::~EmptyViewAdapter() {
	delete d;
}


void EmptyViewAdapter::installEventFilterOnViewWidgets(QObject* object) {
	widget()->installEventFilter(object);
}


Document::Ptr EmptyViewAdapter::document() const {
	return Document::Ptr();
}


void EmptyViewAdapter::setDocument(Document::Ptr) {
}


} // namespace
