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

// KF

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
    ~RasterImageViewAdapter() override;

    QCursor cursor() const override;

    void setCursor(const QCursor &) override;

    MimeTypeUtils::Kind kind() const override
    {
        return MimeTypeUtils::KIND_RASTER_IMAGE;
    }

    bool canZoom() const override
    {
        return true;
    }

    void setZoomToFit(bool) override;

    void setZoomToFill(bool on, const QPointF &center) override;

    bool zoomToFit() const override;

    bool zoomToFill() const override;

    qreal zoom() const override;

    void setZoom(qreal zoom, const QPointF &center) override;

    qreal computeZoomToFit() const override;

    qreal computeZoomToFill() const override;

    Document::Ptr document() const override;

    void setDocument(const Document::Ptr &) override;

    void loadConfig() override;

    RasterImageView *rasterImageView() const override;
    AbstractImageView *imageView() const override;

    QPointF scrollPos() const override;
    void setScrollPos(const QPointF &pos) override;

    QRectF visibleDocumentRect() const override;

private Q_SLOTS:
    void slotLoadingFailed();

private:
    RasterImageViewAdapterPrivate *const d;
};

} // namespace

#endif /* RASTERIMAGEVIEWADAPTER_H */
