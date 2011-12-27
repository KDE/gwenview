// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef RASTERIMAGEVIEWADAPTER_H
#define RASTERIMAGEVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/documentview/abstractdocumentviewadapter.h>

namespace Gwenview
{

struct RasterImageViewAdapterPrivate;
class GWENVIEWLIB_EXPORT RasterImageViewAdapter : public AbstractDocumentViewAdapter
{
    Q_OBJECT
public:
    RasterImageViewAdapter();
    ~RasterImageViewAdapter();

    virtual QCursor cursor() const;

    virtual void setCursor(const QCursor&);

    virtual MimeTypeUtils::Kind kind() const
    {
        return MimeTypeUtils::KIND_RASTER_IMAGE;
    }

    virtual bool canZoom() const
    {
        return true;
    }

    virtual void setZoomToFit(bool);

    virtual bool zoomToFit() const;

    virtual qreal zoom() const;

    virtual void setZoom(qreal zoom, const QPointF& center);

    virtual qreal computeZoomToFit() const;

    virtual Document::Ptr document() const;

    virtual void setDocument(Document::Ptr);

    virtual void loadConfig();

    virtual RasterImageView* rasterImageView() const;

    virtual QPointF scrollPos() const;
    virtual void setScrollPos(const QPointF& pos);

    virtual QRectF visibleDocumentRect() const;

private Q_SLOTS:
    void slotLoadingFailed();

private:
    RasterImageViewAdapterPrivate* const d;
};

} // namespace

#endif /* RASTERIMAGEVIEWADAPTER_H */
