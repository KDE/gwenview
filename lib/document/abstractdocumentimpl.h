// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef ABSTRACTDOCUMENTIMPL_H
#define ABSTRACTDOCUMENTIMPL_H

// Qt
#include <QByteArray>
#include <QObject>

// KDE

// Local
#include <lib/document/document.h>

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

	virtual bool isMetaDataLoaded() const = 0;

	virtual Document::LoadingState loadingState() const = 0;

	virtual Document::SaveResult save(const KUrl&, const QByteArray& format) = 0;

	virtual void setImage(const QImage&) = 0;

	virtual void applyTransformation(Orientation) {}

	virtual QByteArray rawData() const { return QByteArray(); }

Q_SIGNALS:
	void imageRectUpdated(const QRect&);
	void metaDataLoaded();
	void loaded();
	void loadingFailed();

protected:
	Document* document() const;
	void setDocumentImage(const QImage& image);
	void setDocumentImageSize(const QSize& size);
	void setDocumentKind(MimeTypeUtils::Kind);
	void setDocumentFormat(const QByteArray& format);
	void setDocumentExiv2Image(Exiv2::Image::AutoPtr);
	void setDocumentDownSampledImage(const QImage&, int invertedZoom);
	void switchToImpl(AbstractDocumentImpl*  impl);

private:
	AbstractDocumentImplPrivate* const d;
};


} // namespace

#endif /* ABSTRACTDOCUMENTIMPL_H */
