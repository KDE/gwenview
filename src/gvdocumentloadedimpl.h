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
#ifndef GVDOCUMENTLOADEDIMPL_H
#define GVDOCUMENTLOADEDIMPL_H

// Qt
#include <qimage.h>

// Local
#include "gvdocumentimpl.h"

class GVDocument;
class QFile;

class GVDocumentLoadedImpl : public GVDocumentImpl {
Q_OBJECT
public:
	GVDocumentLoadedImpl(GVDocument* document);
	void init();
	~GVDocumentLoadedImpl();
	
	void transform(GVImageUtils::Orientation);
	QString save(const KURL&, const QCString& format) const;

protected:
	virtual QString localSave(QFile* file, const QCString& format) const;
};

#endif /* GVDOCUMENTLOADEDIMPL_H */
