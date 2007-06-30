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
#ifndef ABSTRACTDOCUMENTIMPL_H
#define ABSTRACTDOCUMENTIMPL_H

// Qt
#include <QObject>

// KDE

// Local
#include "document.h"

class QImage;
class QRect;

namespace Gwenview {

class Document;

class AbstractDocumentImplPrivate;
class AbstractDocumentImpl : public QObject {
	Q_OBJECT
public:
	AbstractDocumentImpl(Document*);
	virtual ~AbstractDocumentImpl();

	/**
	 * This method is called by Document::switchToImpl after it has connected
	 * signals to the object
	 */
	virtual void init() = 0;

	virtual bool isLoaded() const = 0;

	virtual Document::SaveResult save(const KUrl&, const QString& format) = 0;

Q_SIGNALS:
	void loaded();

protected:
	Document* document() const;
	void setDocumentImage(const QImage& image);
	void setDocumentFormat(const QByteArray& format);
	void switchToImpl(AbstractDocumentImpl*  impl);

private:
	AbstractDocumentImplPrivate* const d;
};


} // namespace

#endif /* ABSTRACTDOCUMENTIMPL_H */
