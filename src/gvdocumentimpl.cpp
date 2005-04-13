// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
 
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
*/
// KDE
#include <klocale.h>

// Local
#include "gvdocument.h"
#include "gvdocumentimpl.moc"

GVDocumentImpl::GVDocumentImpl(GVDocument* document)
: mDocument(document) {}

void GVDocumentImpl::init() {}

GVDocumentImpl::~GVDocumentImpl() {}

void GVDocumentImpl::switchToImpl(GVDocumentImpl* impl) {
	mDocument->switchToImpl(impl);
}

void GVDocumentImpl::setImage(QImage img, bool update) {
	mDocument->setImage(img, update);
}

void GVDocumentImpl::setImageFormat(const QCString& format) {
	mDocument->setImageFormat(format);
}

void GVDocumentImpl::setFileSize(int size) const {
	mDocument->setFileSize(size);
}

QString GVDocumentImpl::comment() const {
	return QString::null;
}

GVDocument::CommentState GVDocumentImpl::commentState() const {
	return GVDocument::NONE;
}

void GVDocumentImpl::setComment(const QString&) {
}

void GVDocumentImpl::transform(GVImageUtils::Orientation) {
}

QString GVDocumentImpl::save(const KURL&, const QCString&) const {
	return i18n("No document to save");
}
