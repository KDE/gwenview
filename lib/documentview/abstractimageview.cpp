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
// Self
#include "abstractimageview.h"

// Local

// KDE

// Qt
#include <QDebug>
#include <QGuiApplication>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QStandardPaths>

namespace Gwenview
{

static const int UNIT_STEP = 16;

struct AbstractImageViewPrivate
{
    enum Verbosity {
        Silent,
        Notify
    };
    AbstractImageView* q;
    QCursor mZoomCursor;
    Document::Ptr mDocument;

    bool mControlKeyIsDown;
    bool mEnlargeSmallerImages;

    qreal mZoom;
    bool mZoomToFit;
    bool mZoomToFill;
    QPointF mImageOffset;
    QPointF mScrollPos;
    QPointF mLastDragPos;
    QSizeF mDocumentSize;

    void adjustImageOffset(Verbosity verbosity = Notify)
    {
        QSizeF zoomedDocSize = q->documentSize() * mZoom;
        QSizeF viewSize = q->boundingRect().size();
        QPointF offset(
            qMax((viewSize.width() - zoomedDocSize.width()) / 2, qreal(0.)),
            qMax((viewSize.height() - zoomedDocSize.height()) / 2, qreal(0.))
        );
        if (offset != mImageOffset) {
            mImageOffset = offset;
            if (verbosity == Notify) {
                q->onImageOffsetChanged();
            }
        }
    }

    void adjustScrollPos(Verbosity verbosity = Notify)
    {
        setScrollPos(mScrollPos, verbosity);
    }

    void setScrollPos(const QPointF& _newPos, Verbosity verbosity = Notify)
    {
        if (!mDocument) {
            mScrollPos = _newPos;
            return;
        }
        QSizeF zoomedDocSize = q->documentSize() * mZoom;
        QSizeF viewSize = q->boundingRect().size();
        QPointF newPos(
            qBound(qreal(0.), _newPos.x(), zoomedDocSize.width() - viewSize.width()),
            qBound(qreal(0.), _newPos.y(), zoomedDocSize.height() - viewSize.height())
        );
        if (newPos != mScrollPos) {
            QPointF oldPos = mScrollPos;
            mScrollPos = newPos;
            if (verbosity == Notify) {
                q->onScrollPosChanged(oldPos);
            }
            // No verbosity test: we always notify the outside world about
            // scrollPos changes
            QMetaObject::invokeMethod(q, "scrollPosChanged");
        }
    }

    void setupZoomCursor()
    {
        // We do not use "appdata" here because that does not work when this
        // code is called from a KPart.
        const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("gwenview/cursors/zoom.png"));
        QPixmap cursorPixmap = QPixmap(path);
        mZoomCursor = QCursor(cursorPixmap, 11, 11);
    }
};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate)
{
    d->q = this;
    d->mControlKeyIsDown = false;
    d->mEnlargeSmallerImages = false;
    d->mZoom = 1;
    d->mZoomToFit = true;
    d->mZoomToFill = false;
    d->mImageOffset = QPointF(0, 0);
    d->mScrollPos = QPointF(0, 0);
    setFocusPolicy(Qt::WheelFocus);
    setFlag(ItemIsSelectable);
    setAcceptHoverEvents(true);
    d->setupZoomCursor();
    updateCursor();
}

AbstractImageView::~AbstractImageView()
{
    if (d->mDocument) {
        d->mDocument->stopAnimation();
    }
    delete d;
}

Document::Ptr AbstractImageView::document() const
{
    return d->mDocument;
}

void AbstractImageView::setDocument(Document::Ptr doc)
{
    if (d->mDocument) {
        disconnect(d->mDocument.data(), 0, this, 0);
    }
    d->mDocument = doc;
    loadFromDocument();
}

QSizeF AbstractImageView::documentSize() const
{
    return d->mDocument ? d->mDocument->size() : QSizeF();
}

qreal AbstractImageView::zoom() const
{
    return d->mZoom;
}

void AbstractImageView::setZoom(qreal zoom, const QPointF& _center, AbstractImageView::UpdateType updateType)
{
    if (!d->mDocument) {
        d->mZoom = zoom;
        return;
    }

    if (updateType == UpdateIfNecessary
            && qFuzzyCompare(zoom, d->mZoom) && documentSize() == d->mDocumentSize) {
        return;
    }
    qreal oldZoom = d->mZoom;
    d->mZoom = zoom;
    d->mDocumentSize = documentSize();

    QPointF center;
    if (_center == QPointF(-1, -1)) {
        center = boundingRect().center();
    } else {
        center = _center;
    }

    /*
    We want to keep the point at viewport coordinates "center" at the same
    position after zooming. The coordinates of this point in image coordinates
    can be expressed like this:

                          oldScroll + center
    imagePointAtOldZoom = ------------------
                               oldZoom

                       scroll + center
    imagePointAtZoom = ---------------
                            zoom

    So we want:

        imagePointAtOldZoom = imagePointAtZoom

        oldScroll + center   scroll + center
    <=> ------------------ = ---------------
              oldZoom             zoom

                  zoom
    <=> scroll = ------- (oldScroll + center) - center
                 oldZoom
    */

    /*
    Compute oldScroll
    It's useless to take the new offset in consideration because if a direction
    of the new offset is not 0, we won't be able to center on a specific point
    in that direction.
    */
    QPointF oldScroll = scrollPos() - imageOffset();

    QPointF scroll = (zoom / oldZoom) * (oldScroll + center) - center;

    d->adjustImageOffset(AbstractImageViewPrivate::Silent);
    d->setScrollPos(scroll, AbstractImageViewPrivate::Silent);
    onZoomChanged();
    zoomChanged(d->mZoom);
}

bool AbstractImageView::zoomToFit() const
{
    return d->mZoomToFit;
}

bool AbstractImageView::zoomToFill() const
{
    return d->mZoomToFill;
}

void AbstractImageView::setZoomToFit(bool on)
{
    d->mZoomToFit = on;
    if (on) {
        setZoom(computeZoomToFit());
    }
    // We do not set zoom to 1 if zoomToFit is off, this is up to the code
    // calling us. It may went to zoom to some other level and/or to zoom on
    // a particular position
    zoomToFitChanged(d->mZoomToFit);
}

void AbstractImageView::setZoomToFill(bool on)
{
    d->mZoomToFill = on;
    if (on) {
        setZoom(computeZoomToFill());
    }
    // We do not set zoom to 1 if zoomToFit is off, this is up to the code
    // calling us. It may went to zoom to some other level and/or to zoom on
    // a particular position
    zoomToFillChanged(d->mZoomToFill);
}

void AbstractImageView::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    if (d->mZoomToFit) {
        // setZoom() calls adjustImageOffset(), but only if the zoom changes.
        // If the view is resized but does not cause a zoom change, we call
        // adjustImageOffset() ourself.
        const qreal newZoom = computeZoomToFit();
        if (qFuzzyCompare(zoom(), newZoom)) {
            d->adjustImageOffset(AbstractImageViewPrivate::Notify);
        } else {
            setZoom(newZoom);
        }
    } else if (d->mZoomToFill) {
        const qreal newZoom = computeZoomToFill();
        if (qFuzzyCompare(zoom(), newZoom)) {
            d->adjustImageOffset(AbstractImageViewPrivate::Notify);
        } else {
            setZoom(newZoom);
        }
    } else {
        d->adjustImageOffset();
        d->adjustScrollPos();
    }
}

void AbstractImageView::focusInEvent(QFocusEvent* event)
{
    QGraphicsWidget::focusInEvent(event);

    // We might have missed a keyReleaseEvent for the control key, e.g. for Ctrl+O
    const bool controlKeyIsCurrentlyDown = QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier;
    if (d->mControlKeyIsDown != controlKeyIsCurrentlyDown) {
        d->mControlKeyIsDown = controlKeyIsCurrentlyDown;
        updateCursor();
    }
}

qreal AbstractImageView::computeZoomToFit() const
{
    QSizeF docSize = documentSize();
    if (docSize.isEmpty()) {
        return 1;
    }
    QSizeF viewSize = boundingRect().size();
    qreal fitWidth = viewSize.width() / docSize.width();
    qreal fitHeight = viewSize.height() / docSize.height();
    qreal fit = qMin(fitWidth, fitHeight);
    if (!d->mEnlargeSmallerImages) {
        fit = qMin(fit, qreal(1.));
    }
    return fit;
}

qreal AbstractImageView::computeZoomToFill() const
{
    QSizeF docSize = documentSize();
    if (docSize.isEmpty()) {
        return 1;
    }
    QSizeF viewSize = boundingRect().size();
    qreal fitWidth = viewSize.width() / docSize.width();
    qreal fitHeight = viewSize.height() / docSize.height();
    qreal fill = qMax(fitWidth, fitHeight);
    if (!d->mEnlargeSmallerImages) {
        fill = qMin(fill, qreal(1.));
    }
    return fill;
}

void AbstractImageView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mousePressEvent(event);
    if (event->button() == Qt::MiddleButton) {
        bool value = !zoomToFit();
        setZoomToFit(value);
        if (!value) {
            setZoom(1., event->pos());
        }
        return;
    }

    if (event->modifiers() & Qt::ControlModifier) {
        if (event->button() == Qt::LeftButton) {
            zoomInRequested(event->pos());
            return;
        } else if (event->button() == Qt::RightButton) {
            zoomOutRequested(event->pos());
            return;
        }
    }

    d->mLastDragPos = event->pos();
    updateCursor();
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);

    QPointF mousePos = event->pos();
    QPointF newScrollPos = d->mScrollPos + d->mLastDragPos - mousePos;

#if 0 // commented out due to mouse pointer warping around, bug in Qt?
    // Wrap mouse pos
    qreal maxWidth = boundingRect().width();
    qreal maxHeight = boundingRect().height();
    // We need a margin because if the window is maximized, the mouse may not
    // be able to go past the bounding rect.
    // The mouse get placed 1 pixel before/after the margin to avoid getting
    // considered as needing to wrap the other way in next mouseMoveEvent
    // (because we don't check the move vector)
    const int margin = 5;
    if (mousePos.x() <= margin) {
        mousePos.setX(maxWidth - margin - 1);
    } else if (mousePos.x() >= maxWidth - margin) {
        mousePos.setX(margin + 1);
    }
    if (mousePos.y() <= margin) {
        mousePos.setY(maxHeight - margin - 1);
    } else if (mousePos.y() >= maxHeight - margin) {
        mousePos.setY(margin + 1);
    }

    // Set mouse pos (Hackish translation to screen coords!)
    QPointF screenDelta = event->screenPos() - event->pos();
    QCursor::setPos((mousePos + screenDelta).toPoint());
#endif

    d->mLastDragPos = mousePos;
    d->setScrollPos(newScrollPos);
}

void AbstractImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (!d->mLastDragPos.isNull()) {
        d->mLastDragPos = QPointF();
    }
    updateCursor();
}

void AbstractImageView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control) {
        d->mControlKeyIsDown = true;
        updateCursor();
        return;
    }
    if (zoomToFit() || qFuzzyCompare(computeZoomToFit(), zoom())) {
        if (event->modifiers() != Qt::NoModifier) {
            return;
        }

        switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Up:
            previousImageRequested();
            break;
        case Qt::Key_Right:
        case Qt::Key_Down:
            nextImageRequested();
            break;
        default:
            break;
        }
        return;
    }

    QPointF delta(0, 0);
    qreal pageStep = boundingRect().height();
    qreal unitStep;

    if (event->modifiers() & Qt::ShiftModifier) {
        unitStep = pageStep / 2;
    } else {
        unitStep = UNIT_STEP;
    }
    switch (event->key()) {
    case Qt::Key_Left:
        delta.setX(-unitStep);
        break;
    case Qt::Key_Right:
        delta.setX(unitStep);
        break;
    case Qt::Key_Up:
        delta.setY(-unitStep);
        break;
    case Qt::Key_Down:
        delta.setY(unitStep);
        break;
    case Qt::Key_PageUp:
        delta.setY(-pageStep);
        break;
    case Qt::Key_PageDown:
        delta.setY(pageStep);
        break;
    case Qt::Key_Home:
        d->setScrollPos(QPointF(d->mScrollPos.x(), 0));
        return;
    case Qt::Key_End:
        d->setScrollPos(QPointF(d->mScrollPos.x(), documentSize().height() * zoom()));
        return;
    default:
        return;
    }
    d->setScrollPos(d->mScrollPos + delta);
}

void AbstractImageView::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control) {
        d->mControlKeyIsDown = false;
        updateCursor();
    }
}

void AbstractImageView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->modifiers() == Qt::NoModifier) {
        toggleFullScreenRequested();
    }
}

QPointF AbstractImageView::imageOffset() const
{
    return d->mImageOffset;
}

QPointF AbstractImageView::scrollPos() const
{
    return d->mScrollPos;
}

void AbstractImageView::setScrollPos(const QPointF& pos)
{
    d->setScrollPos(pos);
}

QPointF AbstractImageView::mapToView(const QPointF& imagePos) const
{
    return imagePos * d->mZoom + d->mImageOffset - d->mScrollPos;
}

QPoint AbstractImageView::mapToView(const QPoint& imagePos) const
{
    return mapToView(QPointF(imagePos)).toPoint();
}

QRectF AbstractImageView::mapToView(const QRectF& imageRect) const
{
    return QRectF(
               mapToView(imageRect.topLeft()),
               imageRect.size() * zoom()
           );
}

QRect AbstractImageView::mapToView(const QRect& imageRect) const
{
    return QRect(
               mapToView(imageRect.topLeft()),
               imageRect.size() * zoom()
           );
}

QPointF AbstractImageView::mapToImage(const QPointF& viewPos) const
{
    return (viewPos - d->mImageOffset + d->mScrollPos) / d->mZoom;
}

QPoint AbstractImageView::mapToImage(const QPoint& viewPos) const
{
    return mapToImage(QPointF(viewPos)).toPoint();
}

QRectF AbstractImageView::mapToImage(const QRectF& viewRect) const
{
    return QRectF(
               mapToImage(viewRect.topLeft()),
               viewRect.size() / zoom()
           );
}

QRect AbstractImageView::mapToImage(const QRect& viewRect) const
{
    return QRect(
               mapToImage(viewRect.topLeft()),
               viewRect.size() / zoom()
           );
}

void AbstractImageView::setEnlargeSmallerImages(bool value)
{
    d->mEnlargeSmallerImages = value;
    if (zoomToFit()) {
        setZoom(computeZoomToFit());
    }
}

void AbstractImageView::updateCursor()
{
    if (d->mControlKeyIsDown) {
        setCursor(d->mZoomCursor);
    } else {
        if (d->mLastDragPos.isNull()) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ClosedHandCursor);
        }
    }
}

QSizeF AbstractImageView::visibleImageSize() const
{
    if (!document()) {
        return QSizeF();
    }
    QSizeF size = documentSize() * zoom();
    return size.boundedTo(boundingRect().size());
}

void AbstractImageView::applyPendingScrollPos()
{
    d->adjustImageOffset();
    d->adjustScrollPos();
}

} // namespace
