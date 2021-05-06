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

// KF

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
    explicit SvgImageView(QGraphicsItem* parent = nullptr);

protected:
    void loadFromDocument() override;
    void onZoomChanged() override;
    void onImageOffsetChanged() override;
    void onScrollPosChanged(const QPointF& oldPos) override;

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
    ~SvgViewAdapter() override;

    QCursor cursor() const override;

    void setCursor(const QCursor&) override;

    void setDocument(const Document::Ptr &) override;

    Document::Ptr document() const override;

    void loadConfig() override;

    MimeTypeUtils::Kind kind() const override
    {
        return MimeTypeUtils::KIND_SVG_IMAGE;
    }

    bool canZoom() const override
    {
        return true;
    }

    void setZoomToFit(bool) override;

    void setZoomToFill(bool on, const QPointF& center) override;

    bool zoomToFit() const override;

    bool zoomToFill() const override;

    qreal zoom() const override;

    void setZoom(qreal /*zoom*/, const QPointF& /*center*/ = QPointF(-1, -1)) override;

    qreal computeZoomToFit() const override;

    qreal computeZoomToFill() const override;

    QPointF scrollPos() const override;
    void setScrollPos(const QPointF& pos) override;

    QRectF visibleDocumentRect() const override;

    AbstractImageView* imageView() const override;

private:
    SvgViewAdapterPrivate* const d;
};

} // namespace

#endif /* SVGVIEWADAPTER_H */
