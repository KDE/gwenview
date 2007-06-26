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
#ifndef DOCUMENTIMPL_H
#define DOCUMENTIMPL_H

// Qt
#include <qobject.h>
#include <qrect.h>

// Local
#include "document.h"
#include "imageutils/orientation.h"
namespace Gwenview {


class DocumentImpl : public QObject {
Q_OBJECT
public:
	DocumentImpl(Document* document);
	virtual ~DocumentImpl();
	/**
	 * This method is called by Document::switchToImpl after it has connect
	 * signals to the object
	 */
	virtual void init();
	
	void switchToImpl(DocumentImpl*);
	void setImage(QImage);
	void setMimeType(const QString&);
	void setImageFormat(const QCString&);
	void setFileSize(int) const;

	/**
	 * Convenience method to emit rectUpdated with the whole image rect
	 */
	void emitImageRectUpdated();

	virtual QString aperture() const;
	virtual QString exposureTime() const;
	virtual QString iso() const;
	virtual QString focalLength() const;

	virtual QString comment() const;
	virtual Document::CommentState commentState() const;
	virtual void setComment(const QString&);
	virtual int duration() const;
	
	virtual void transform(ImageUtils::Orientation);
	virtual QString save(const KURL&, const QCString& format) const;

	virtual MimeTypeUtils::Kind urlKind() const=0;

	virtual bool canBeSaved() const=0;


signals:
	void finished(bool success);
	void sizeUpdated();
	void rectUpdated(const QRect&);
	
protected:
	Document* mDocument;
};

class DocumentEmptyImpl : public DocumentImpl {
public:
	DocumentEmptyImpl(Document* document)
	: DocumentImpl(document) {
		setImage(QImage());
		setImageFormat(0);
		setMimeType("application/x-zerosize");
	}

	MimeTypeUtils::Kind urlKind() const {
		return MimeTypeUtils::KIND_UNKNOWN;
	}

	bool canBeSaved() const {
		return false;
	}
};

} // namespace
#endif /* DOCUMENTIMPL_H */

