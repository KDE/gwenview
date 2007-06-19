// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef DOCUMENTLOADEDIMPL_H
#define DOCUMENTLOADEDIMPL_H

// Qt

// KDE

// Local
#include "abstractdocumentimpl.h"

class QIODevice;

namespace Gwenview {


class DocumentLoadedImplPrivate;
class DocumentLoadedImpl : public AbstractDocumentImpl {
public:
	DocumentLoadedImpl(Document*);
	~DocumentLoadedImpl();

	virtual void init();
	virtual bool isLoaded() const;
	virtual Document::SaveResult save(const KUrl&, const QString& format);

protected:
	virtual bool saveInternal(QIODevice* device, const QString& format);

private:
	DocumentLoadedImplPrivate* const d;
};


} // namespace

#endif /* DOCUMENTLOADEDIMPL_H */
