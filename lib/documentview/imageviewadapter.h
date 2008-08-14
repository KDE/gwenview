// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef IMAGEVIEWADAPTER_H
#define IMAGEVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/documentview/abstractdocumentviewadapter.h>

namespace Gwenview {


class ImageViewAdapterPrivate;
class GWENVIEWLIB_EXPORT ImageViewAdapter : public AbstractDocumentViewAdapter {
	Q_OBJECT
public:
	ImageViewAdapter(QWidget*);
	~ImageViewAdapter();

	virtual void installEventFilterOnViewWidgets(QObject*);

	virtual MimeTypeUtils::Kind kind() const { return MimeTypeUtils::KIND_RASTER_IMAGE; }

	virtual bool canZoom() const { return true; }

	virtual void setZoomToFit(bool);

	virtual bool zoomToFit() const;

	virtual qreal zoom() const;

	virtual void setZoom(qreal zoom, const QPoint& center);

	virtual qreal computeZoomToFit() const;

	virtual qreal computeZoomToFitWidth() const;

	virtual qreal computeZoomToFitHeight() const;

	virtual Document::Ptr document() const;

	virtual void setDocument(Document::Ptr);

	virtual ImageView* imageView() const;

	virtual void loadConfig();

private Q_SLOTS:
	void slotLoadingFailed();
	void slotLoaded();

private:
	ImageViewAdapterPrivate* const d;
};


} // namespace

#endif /* IMAGEVIEWADAPTER_H */
