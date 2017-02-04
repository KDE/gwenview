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
#ifndef SVGVIEWADAPTER_H
#define SVGVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGraphicsWidget>

// KDE

// Local
#include <lib/documentview/abstractimageview.h>
#include <lib/documentview/abstractdocumentviewadapter.h>

class QGraphicsSvgItem;

namespace Gwenview
{

class SvgImageView : public AbstractImageView
{
    Q_OBJECT
public:
    SvgImageView(QGraphicsItem* parent = 0);

protected:
    void loadFromDocument() Q_DECL_OVERRIDE;
    void onZoomChanged() Q_DECL_OVERRIDE;
    void onImageOffsetChanged() Q_DECL_OVERRIDE;
    void onScrollPosChanged(const QPointF& oldPos) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void finishLoadFromDocument();

private:
    QGraphicsSvgItem* mSvgItem;
    void adjustItemPos();
};

struct SvgViewAdapterPrivate;
class GWENVIEWLIB_EXPORT SvgViewAdapter : public AbstractDocumentViewAdapter
{
    Q_OBJECT
public:
    SvgViewAdapter();
    ~SvgViewAdapter();

    virtual QCursor cursor() const Q_DECL_OVERRIDE;

    virtual void setCursor(const QCursor&) Q_DECL_OVERRIDE;

    virtual void setDocument(Document::Ptr) Q_DECL_OVERRIDE;

    virtual Document::Ptr document() const Q_DECL_OVERRIDE;

    virtual MimeTypeUtils::Kind kind() const Q_DECL_OVERRIDE
    {
        return MimeTypeUtils::KIND_SVG_IMAGE;
    }

    virtual bool canZoom() const Q_DECL_OVERRIDE
    {
        return true;
    }

    virtual void setZoomToFit(bool) Q_DECL_OVERRIDE;

    virtual void setZoomToFitWidth(bool) Q_DECL_OVERRIDE;

    virtual bool zoomToFit() const Q_DECL_OVERRIDE;

    virtual bool zoomToFitWidth() const Q_DECL_OVERRIDE;

    virtual qreal zoom() const Q_DECL_OVERRIDE;

    virtual void setZoom(qreal /*zoom*/, const QPointF& /*center*/ = QPointF(-1, -1)) Q_DECL_OVERRIDE;

    virtual qreal computeZoomToFit() const Q_DECL_OVERRIDE;

    virtual qreal computeZoomToFitWidth() const Q_DECL_OVERRIDE;

    virtual QPointF scrollPos() const Q_DECL_OVERRIDE;
    virtual void setScrollPos(const QPointF& pos) Q_DECL_OVERRIDE;

private:
    SvgViewAdapterPrivate* const d;
};

} // namespace

#endif /* SVGVIEWADAPTER_H */
