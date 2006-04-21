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
#ifndef DOCUMENTLOADINGIMPL_H
#define DOCUMENTLOADINGIMPL_H

// Local 
#include "documentimpl.h"
#include "mimetypeutils.h"

namespace Gwenview {

class Document;

class DocumentLoadingImplPrivate;

class DocumentLoadingImpl : public DocumentImpl {
Q_OBJECT
public:
	DocumentLoadingImpl(Document* document);
	~DocumentLoadingImpl();
	virtual void init();
	virtual MimeTypeUtils::Kind urlKind() const { return MimeTypeUtils::KIND_RASTER_IMAGE; }
	
private:
	DocumentLoadingImplPrivate* d;

private slots:
	void slotURLKindDetermined();
	void sizeLoaded(int, int);
	void imageChanged(const QRect&);
	void frameLoaded();
	void imageLoaded( bool ok );
};

} // namespace
#endif /* DOCUMENTLOADINGIMPL_H */

