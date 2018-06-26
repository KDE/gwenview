// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef ABSTRACTIMAGEVIEW_H
#define ABSTRACTIMAGEVIEW_H

// Local
#include <lib/document/document.h>

// KDE

// Qt
#include <QGraphicsWidget>

namespace Gwenview
{

struct AbstractImageViewPrivate;
/**
 *
 */
class AbstractImageView : public QGraphicsWidget
{
    Q_OBJECT
public:
    enum UpdateType {
        UpdateIfNecessary,
        ForceUpdate
    };
    enum AlphaBackgroundMode {
        AlphaBackgroundNone,
        AlphaBackgroundCheckBoard,
        AlphaBackgroundSolid
    };

    AbstractImageView(QGraphicsItem* parent);
    ~AbstractImageView() Q_DECL_OVERRIDE;

    qreal zoom() const;

    virtual void setZoom(qreal zoom, const QPointF& center = QPointF(-1, -1), UpdateType updateType = UpdateIfNecessary);

    bool zoomToFit() const;

    bool zoomToFill() const;

    virtual void setZoomToFit(bool value);

    virtual void setZoomToFill(bool value);

    virtual void setDocument(Document::Ptr doc);

    Document::Ptr document() const;

    qreal computeZoomToFit() const;

    qreal computeZoomToFill() const;

    QSizeF documentSize() const;

    QSizeF visibleImageSize() const;

    /**
     * If the image is smaller than the view, imageOffset is the distance from
     * the topleft corner of the view to the topleft corner of the image.
     * Neither x nor y can be negative.
     */
    QPointF imageOffset() const;

    /**
     * The scroll position, in zoomed image coordinates.
     * x and y are always between 0 and (docsize * zoom - viewsize)
     */
    QPointF scrollPos() const;
    void setScrollPos(const QPointF& pos);

    QPointF mapToView(const QPointF& imagePos) const;
    QPoint mapToView(const QPoint& imagePos) const;
    QRectF mapToView(const QRectF& imageRect) const;
    QRect mapToView(const QRect& imageRect) const;

    QPointF mapToImage(const QPointF& viewPos) const;
    QPoint mapToImage(const QPoint& viewPos) const;
    QRectF mapToImage(const QRectF& viewRect) const;
    QRect mapToImage(const QRect& viewRect) const;

    void setEnlargeSmallerImages(bool value);

    void applyPendingScrollPos();

    void resetDragCursor();

public Q_SLOTS:
    void updateCursor();

Q_SIGNALS:
    void zoomToFitChanged(bool);
    void zoomToFillChanged(bool);
    void zoomChanged(qreal);
    void zoomInRequested(const QPointF&);
    void zoomOutRequested(const QPointF&);
    void scrollPosChanged();
    void completed();
    void previousImageRequested();
    void nextImageRequested();
    void toggleFullScreenRequested();

protected:
    virtual void setAlphaBackgroundMode(AlphaBackgroundMode mode) = 0;
    virtual void setAlphaBackgroundColor(const QColor& color) = 0;
    const QPixmap& alphaBackgroundTexture() const;

    virtual void loadFromDocument() = 0;
    virtual void onZoomChanged() = 0;
    /**
     * Called when the offset changes.
     * Note: to avoid multiple adjustments, this is not called if zoom changes!
     */
    virtual void onImageOffsetChanged() = 0;
    /**
     * Called when the scrollPos changes.
     * Note: to avoid multiple adjustments, this is not called if zoom changes!
     */
    virtual void onScrollPosChanged(const QPointF& oldPos) = 0;

    void resizeEvent(QGraphicsSceneResizeEvent* event) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) Q_DECL_OVERRIDE;

private:
    friend struct AbstractImageViewPrivate;
    AbstractImageViewPrivate* const d;
};

} // namespace

#endif /* ABSTRACTIMAGEVIEW_H */
