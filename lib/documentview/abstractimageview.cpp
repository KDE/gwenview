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
#include "abstractimageview.moc"

// Local

// KDE
#include <KDebug>
#include <KModifierKeyInfo>
#include <KStandardDirs>
#include <KUrl>

// Qt
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>

namespace Gwenview
{

static const int UNIT_STEP = 16;

static void restoreAndSetOverrideCursor(const Qt::CursorShape& newShape)
{
    if (!QApplication::overrideCursor()) {
        QApplication::setOverrideCursor(newShape);
        return;
    }
    const Qt::CursorShape currentShape = QApplication::overrideCursor()->shape();
    if (newShape == currentShape) {
        return;
    }
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    QApplication::setOverrideCursor(newShape);
}

struct AbstractImageViewPrivate
{
    enum Verbosity {
        Silent,
        Notify
    };
    AbstractImageView* q;
    KModifierKeyInfo* mModifierKeyInfo;
    QCursor mZoomCursor;
    Document::Ptr mDocument;

    bool mEnlargeSmallerImages;

    qreal mZoom;
    bool mZoomToFit;
    QPointF mImageOffset;
    QPointF mScrollPos;
    QPointF mStartDragPos;
    QPoint mScreenCenter;

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
        QString path = KStandardDirs::locate("data", "gwenview/cursors/zoom.png");
        QPixmap cursorPixmap = QPixmap(path);
        mZoomCursor = QCursor(cursorPixmap, 11, 11);
    }
};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, d(new AbstractImageViewPrivate)
{
    d->q = this;
    d->mEnlargeSmallerImages = false;
    d->mZoom = 1;
    d->mZoomToFit = true;
    d->mImageOffset = QPointF(0, 0);
    d->mScrollPos = QPointF(0, 0);
    d->mModifierKeyInfo = new KModifierKeyInfo(this);
    connect(d->mModifierKeyInfo, SIGNAL(keyPressed(Qt::Key,bool)), SLOT(updateCursor()));
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
    if (updateType == UpdateIfNecessary && qFuzzyCompare(zoom, d->mZoom)) {
        return;
    }
    qreal oldZoom = d->mZoom;
    d->mZoom = zoom;

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
    } else {
        d->adjustImageOffset();
        d->adjustScrollPos();
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

void AbstractImageView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mousePressEvent(event);
    if (event->button() == Qt::MiddleButton) {
        bool value = !zoomToFit();
        setZoomToFit(value);
        if (!value) {
            setZoom(1.);
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

    // Start panning if image is only partially visible and left mouse button is pressed
    if (visibleImageSize() != documentSize() * zoom() && event->button() == Qt::LeftButton) {
        d->mStartDragPos = QCursor::pos();
        QPointF screenCenter = event->screenPos() - event->pos() + QPointF(boundingRect().width()/2., boundingRect().height()/2.);
        d->mScreenCenter = screenCenter.toPoint();
        QCursor::setPos(d->mScreenCenter);
    }

    updateCursor();
}

void AbstractImageView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
    updateCursor();

    // Don't pan if whole image is visible
    if (visibleImageSize() == documentSize() * zoom()) {
        return;
    }

    // Don't pan if Ctrl key is pressed
    if (event->modifiers() & Qt::ControlModifier) {
        return;
    }

    // delta has to be a QPoint (not a QPointF)
    // otherwise d->setScrollPos will be slow.
    QPoint delta = d->mScreenCenter - QCursor::pos();
    QPointF newScrollPos = d->mScrollPos + delta;
    QCursor::setPos(d->mScreenCenter);

    d->setScrollPos(newScrollPos);
}

void AbstractImageView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (!d->mStartDragPos.isNull()) {
        QCursor::setPos(d->mStartDragPos.toPoint());
        d->mStartDragPos = QPointF();
    }
    updateCursor();
}

void AbstractImageView::keyPressEvent(QKeyEvent* event)
{
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

    updateCursor();
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
    if (d->mModifierKeyInfo->isKeyPressed(Qt::Key_Control)) {
        QApplication::restoreOverrideCursor();
        setCursor(d->mZoomCursor);
    } else {
        if (d->mStartDragPos.isNull()) {
            if (QApplication::mouseButtons() == Qt::LeftButton) {
                restoreAndSetOverrideCursor(Qt::ClosedHandCursor);
            } else {
                QApplication::restoreOverrideCursor();
                setCursor(Qt::OpenHandCursor);
            }
        } else {
            if (visibleImageSize() != documentSize() * zoom()) {
                restoreAndSetOverrideCursor(Qt::BlankCursor);
            }
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
