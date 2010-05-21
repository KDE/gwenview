// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "abstractimageoperation.h"

// Qt

// KDE
#include <kurl.h>

// Local
#include "document/documentfactory.h"

namespace Gwenview {


struct AbstractImageOperationPrivate {
	KUrl mUrl;
};


AbstractImageOperation::AbstractImageOperation()
: QUndoCommand()
, d(new AbstractImageOperationPrivate) {
}


AbstractImageOperation::~AbstractImageOperation() {
	delete d;
}


void AbstractImageOperation::applyToDocument(Document::Ptr doc) {
	d->mUrl = doc->url();
	doc->undoStack()->push(this);
}


Document::Ptr AbstractImageOperation::document() const {
	Document::Ptr doc = DocumentFactory::instance()->load(d->mUrl);
	doc->loadFullImage();
	return doc;
}


} // namespace
