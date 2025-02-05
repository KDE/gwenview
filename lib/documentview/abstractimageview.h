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

// KF

// Qt
#include <QGraphicsWidget>

namespace Gwenview
{
class AlphaBackgroundItem;

struct AbstractImageViewPrivate;
/**
 * The abstract base class used to implement common functionality of raster and vector image views
 * like for example zooming, computing the area where the image will be displayed in the view and
 * dealing with a background for transparent areas.
 */
class AbstractImageView : public QGraphicsWidget
{
    Q_OBJECT
public:
    enum UpdateType {
        UpdateIfNecessary,
        ForceUpdate,
    };
    enum AlphaBackgroundMode {
        AlphaBackgroundNone,
        AlphaBackgroundCheckBoard,
        AlphaBackgroundSolid,
    };

    AbstractImageView(QGraphicsItem *parent);
    ~AbstractImageView() override;

    qreal zoom() const;

    virtual void setZoom(qreal zoom, const QPointF &center = QPointF(-1, -1), UpdateType updateType = UpdateIfNecessary);

    bool zoomToFit() const;

    bool zoomToFill() const;

    virtual void setZoomToFit(bool value);

    virtual void setZoomToFill(bool value, const QPointF &center = QPointF(-1, -1));

    virtual void setDocument(const Document::Ptr &doc);

    Document::Ptr document() const;

    qreal computeZoomToFit() const;

    qreal computeZoomToFill() const;

    QSizeF documentSize() const;

    /**
     * Returns the size of the loaded document in device independent pixels.
     */
    QSizeF dipDocumentSize() const;

    /*
     * The size of the image that is currently visible,
     * in device independent pixels.
     */
    QSizeF visibleImageSize() const;

    /**
     * If the image is smaller than the view, imageOffset is the distance from
     * the topleft corner of the view to the topleft corner of the image.
     * Neither x nor y can be negative.
     * This is in device independent pixels.
     */
    QPointF imageOffset() const;

    /**
     * The scroll position, in zoomed image coordinates.
     * x and y are always between 0 and (docsize * zoom - viewsize)
     */
    QPointF scrollPos() const;
    void setScrollPos(const QPointF &pos);

    qreal devicePixelRatio() const;
    void onDevicePixelRatioChange();

    QPointF mapToView(const QPointF &imagePos) const;
    QPoint mapToView(const QPoint &imagePos) const;
    QRectF mapToView(const QRectF &imageRect) const;
    QRect mapToView(const QRect &imageRect) const;

    QPointF mapToImage(const QPointF &viewPos) const;
    QPoint mapToImage(const QPoint &viewPos) const;
    QRectF mapToImage(const QRectF &viewRect) const;
    QRect mapToImage(const QRect &viewRect) const;

    void setEnlargeSmallerImages(bool value);

    void applyPendingScrollPos();

    void resetDragCursor();

    AlphaBackgroundItem *backgroundItem() const;

public Q_SLOTS:
    void updateCursor();

Q_SIGNALS:
    /** Emitted when the zoom mode changes to or from "Fit". */
    void zoomToFitChanged(bool);
    /** Emitted when the zoom mode changes to or from "Fill". */
    void zoomToFillChanged(bool);
    /** Emitted when the zoom value changes in any way. */
    void zoomChanged(qreal);
    void zoomInRequested(const QPointF &);
    void zoomOutRequested(const QPointF &);
    void scrollPosChanged();
    void completed();
    void previousImageRequested();
    void nextImageRequested();
    void toggleFullScreenRequested();

protected:
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
    virtual void onScrollPosChanged(const QPointF &oldPos) = 0;

    void onImageRectUpdated();

    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    friend struct AbstractImageViewPrivate;
    AbstractImageViewPrivate *const d;
};

} // namespace

#endif /* ABSTRACTIMAGEVIEW_H */
