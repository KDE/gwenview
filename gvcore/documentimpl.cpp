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
#include "document.h"
#include "documentimpl.moc"
namespace Gwenview {

DocumentImpl::DocumentImpl(Document* document)
: mDocument(document) {}

void DocumentImpl::init() {}

DocumentImpl::~DocumentImpl() {}

void DocumentImpl::switchToImpl(DocumentImpl* impl) {
	mDocument->switchToImpl(impl);
}

void DocumentImpl::setImage(QImage img) {
	mDocument->setImage(img);
}

void DocumentImpl::emitImageRectUpdated() {
	emit rectUpdated(mDocument->image().rect());
}

void DocumentImpl::setImageFormat(const QCString& format) {
	mDocument->setImageFormat(format);
}

void DocumentImpl::setFileSize(int size) const {
	mDocument->setFileSize(size);
}

QString DocumentImpl::comment() const {
	return QString::null;
}

Document::CommentState DocumentImpl::commentState() const {
	return Document::NONE;
}

void DocumentImpl::setComment(const QString&) {
}

int DocumentImpl::duration() const {
	return 0;
}

void DocumentImpl::transform(ImageUtils::Orientation) {
}

QString DocumentImpl::save(const KURL&, const QCString&) const {
	return i18n("No document to save");
}

} // namespace
