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
#ifndef ABSTRACTDOCUMENTIMPL_H
#define ABSTRACTDOCUMENTIMPL_H

// Qt
#include <QByteArray>
#include <QObject>

// KDE

// Local
#include <lib/document/document.h>
#include <lib/orientation.h>

class QImage;
class QRect;

namespace Gwenview {

class Document;
class AbstractDocumentEditor;

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

	virtual Document::LoadingState loadingState() const = 0;

	virtual bool save(const KUrl&, const QByteArray& /*format*/) { return false; }

	virtual AbstractDocumentEditor* editor() { return 0; }

	virtual QByteArray rawData() const { return QByteArray(); }

	virtual bool isEditable() const { return false; }

	virtual bool isAnimated() const { return false; }

	virtual void startAnimation() {}

	virtual void stopAnimation() {}

Q_SIGNALS:
	void imageRectUpdated(const QRect&);
	void metaInfoLoaded();
	void loaded();
	void loadingFailed();
	void isAnimatedUpdated();
	void editorUpdated();

protected:
	Document* document() const;
	void setDocumentImage(const QImage& image);
	void setDocumentImageSize(const QSize& size);
	void setDocumentKind(MimeTypeUtils::Kind);
	void setDocumentFormat(const QByteArray& format);
	void setDocumentExiv2Image(Exiv2::Image::AutoPtr);
	void setDocumentDownSampledImage(const QImage&, int invertedZoom);
	void setDocumentErrorString(const QString&);
	void switchToImpl(AbstractDocumentImpl*  impl);

private:
	AbstractDocumentImplPrivate* const d;
};


} // namespace

#endif /* ABSTRACTDOCUMENTIMPL_H */
