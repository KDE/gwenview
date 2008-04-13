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

#include <string.h>
#include <exiv2/image.hpp>

// Qt
#include <QObject>
#include <QSharedData>
#include <QSize>

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
	static const qreal MaxDownSampledZoom = 0.5;

	enum SaveResult {
		SR_OK,
		SR_ReadOnly,
		SR_UploadFailed,
		SR_OtherError
	};

	enum LoadingState {
		Loading,
		Loaded,
		MetaDataLoaded,
		LoadingFailed
	};

	typedef KSharedPtr<Document> Ptr;
	~Document();

	void reload();

	void loadFullImage();

	/**
	 * Prepare a version of the image down sampled to be a bit bigger than
	 * size() * @a zoom.
	 * Do not ask for a down sampled image for @a zoom >= to MaxDownSampledZoom.
	 *
	 * @return true if the image is ready, false if not. In this case the
	 * downSampledImageReady() signal will be emitted.
	 */
	bool prepareDownSampledImageForZoom(qreal zoom);

	LoadingState loadingState() const;

	bool isModified() const;

	QImage& image();

	const QImage& downSampledImage(qreal zoom) const;

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

	void waitUntilMetaDataLoaded() const;

	void waitUntilLoaded() const;

	const Exiv2::Image* exiv2Image() const;

	QSize size() const;

	int width() const { return size().width(); }

	int height() const { return size().height(); }

	bool hasAlphaChannel() const;

	ImageMetaInfoModel* metaInfo() const;

	QUndoStack* undoStack() const;

Q_SIGNALS:
	void downSampledImageReady();
	void imageRectUpdated(const QRect&);
	void loaded(const KUrl&);
	void loadingFailed(const KUrl&);
	void saved(const KUrl&);
	void modified(const KUrl&);
	void metaDataUpdated();

private Q_SLOTS:
	void emitLoaded();
	void emitLoadingFailed();
	void slotUndoIndexChanged();

private:
	friend class DocumentFactory;
	friend class AbstractDocumentImpl;

	void setImageInternal(const QImage&);
	void setFormat(const QByteArray&);
	void setSize(const QSize&);
	void setExiv2Image(Exiv2::Image::AutoPtr);
	void setDownSampledImage(const QImage&, int invertedZoom);
	void switchToImpl(AbstractDocumentImpl* impl);

	Document(const KUrl&);
	DocumentPrivate * const d;
};

} // namespace

#endif /* DOCUMENT_H */
