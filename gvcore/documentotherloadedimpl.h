// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurelien Gateau
 
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
#ifndef DOCUMENTOTHERLOADEDIMPL_H
#define DOCUMENTOTHERLOADEDIMPL_H

// Local
#include "document.h"
#include "documentimpl.h"

namespace Gwenview {
class Document;

class DocumentOtherLoadedImpl : public DocumentImpl {
public:
	DocumentOtherLoadedImpl(Document* document)
	: DocumentImpl(document) {
		setImage(QImage());
		setImageFormat(0);
	}

	void init() {
		emit finished(true);
	}

	virtual MimeTypeUtils::Kind urlKind() const { return MimeTypeUtils::KIND_FILE; }

	virtual bool canBeSaved() const { return false; }

	virtual int duration() const;
};

} // namespace
#endif /* DOCUMENTOTHERLOADEDIMPL_H */

