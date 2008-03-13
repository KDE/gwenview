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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <lib/gwenviewlib_export.h>

#include <exiv2/image.hpp>

// Qt
#include <QObject>
#include <QSharedData>

// KDE
#include <ksharedptr.h>

// Local
#include <lib/orientation.h>

class QImage;
class QRect;
class QSize;
class QUndoStack;

class KUrl;

namespace Gwenview {

class AbstractDocumentImpl;
class DocumentFactory;
class DocumentPrivate;
class ImageMetaInfoModel;

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

	bool isLoaded() const;

	bool isModified() const;

	QImage& image();

	/**
	 * Replaces the current image with image.
	 * Calling this while the document is loaded won't do anything.
	 *
	 * This method should only be called from a subclass of
	 * AbstractImageOperation and applied through Document::undoStack().
	 */
	void setImage(const QImage& image);

	/**
	 * Apply a transformation to the document image.
	 *
	 * Transformations are handled by the Document class because it can be
	 * done in a lossless way by some Document implementations.
	 *
	 * This method should only be called from a subclass of
	 * AbstractImageOperation and applied through Document::undoStack().
	 */
	void applyTransformation(Orientation);

	KUrl url() const;

	SaveResult save(const KUrl& url, const QByteArray& format);

	QByteArray format() const;

	void waitUntilLoaded() const;

	const Exiv2::Image* exiv2Image() const;

	QSize size() const;

	ImageMetaInfoModel* metaInfo() const;

	QUndoStack* undoStack() const;

Q_SIGNALS:
	void imageRectUpdated(const QRect&);
	void loaded(const KUrl&);
	void saved(const KUrl&);
	void modified(const KUrl&);
	void metaDataUpdated();

private Q_SLOTS:
	void emitLoaded();
	void slotUndoIndexChanged();

private:
	friend class DocumentFactory;
	friend class AbstractDocumentImpl;

	void setImageInternal(const QImage&);
	void setFormat(const QByteArray&);
	void setSize(const QSize&);
	void setExiv2Image(Exiv2::Image::AutoPtr);
	void switchToImpl(AbstractDocumentImpl* impl);

	Document();
	DocumentPrivate * const d;
};

} // namespace

#endif /* DOCUMENT_H */
