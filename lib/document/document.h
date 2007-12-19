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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "../gwenviewlib_export.h"

#include <exiv2/image.hpp>

// Qt
#include <QObject>
#include <QSharedData>

// KDE
#include <ksharedptr.h>

// Local
#include "../orientation.h"

class QImage;
class QRect;
class QSize;

class KUrl;

namespace Gwenview {

class AbstractDocumentImpl;
class DocumentFactory;
class DocumentPrivate;

class GWENVIEWLIB_EXPORT Document : public QObject, public QSharedData {
	Q_OBJECT
public:
	enum SaveResult {
		SR_OK,
		SR_ReadOnly,
		SR_UploadFailed,
		SR_OtherError
	};

	typedef KSharedPtr<Document> Ptr;
	~Document();
	void load(const KUrl&);

	void reload();

	bool isMetaDataLoaded() const;

	bool isLoaded() const;

	bool isModified() const;

	/**
	 * Mark the image as modified. Should be called when image pixels have been
	 * altered outside Document.
	 */
	void setModified(bool modified);

	QImage& image();

	/**
	 * Replaces the current image with image.
	 * Calling this while the document is loaded won't do anything.
	 * isModified() will return true after this.
	 */
	void setImage(const QImage& image);

	/**
	 * Apply a transformation to the document image.
	 *
	 * Transformations are handled by the Document class because it can be
	 * done in a lossless way by some Document implementations.
	 */
	void applyTransformation(Orientation);

	KUrl url() const;

	SaveResult save(const KUrl& url, const QByteArray& format);

	QByteArray format() const;

	void waitUntilLoaded() const;

	const Exiv2::Image* exiv2Image() const;

	QSize size() const;

Q_SIGNALS:
	void imageRectUpdated(const QRect&);
	void loaded(const KUrl&);
	void saved(const KUrl&);
	void modified(const KUrl&);
	void metaDataLoaded();

private Q_SLOTS:
	void emitLoaded();

private:
	friend class DocumentFactory;
	friend class AbstractDocumentImpl;

	void setImageInternal(const QImage&);
	void setFormat(const QByteArray&);
	void setSize(const QSize&);
	void setExiv2Image(Exiv2::Image::AutoPtr);
	void switchToImpl(AbstractDocumentImpl* impl);

	void emitMetaDataLoaded();

	Document();
	DocumentPrivate * const d;
};

} // namespace

#endif /* DOCUMENT_H */
