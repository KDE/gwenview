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
#include "birdeyeview.h"

// Local
#include "gwenview_lib_debug.h"
#include <lib/document/document.h>
#include <lib/documentview/documentview.h>

// KF

// Qt
#include <QApplication>
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>

namespace Gwenview
{
static qreal MIN_SIZE = 72;
static qreal VIEW_OFFSET = MIN_SIZE / 4;

static int AUTOHIDE_DELAY = 2000;

/**
 * Returns a QRectF whose coordinates are rounded to completely contains rect
 */
inline QRectF alignedRectF(const QRectF &rect)
{
    return QRectF(rect.toAlignedRect());
}

struct BirdEyeViewPrivate {
    BirdEyeView *q = nullptr;
    DocumentView *mDocView = nullptr;
    QPropertyAnimation *mOpacityAnim = nullptr;
    QTimer *mAutoHideTimer = nullptr;
    QRectF mVisibleRect;
    QPointF mStartDragMousePos;
    QPointF mStartDragViewPos;

    void updateCursor(const QPointF &pos)
    {
        q->setCursor(mVisibleRect.contains(pos) ? Qt::OpenHandCursor : Qt::ArrowCursor);
    }

    void updateVisibility()
    {
        bool visible;
        if (!mDocView->canZoom() || mDocView->zoomToFit()) {
            // No need to show
            visible = false;
        } else if (mDocView->isAnimated()) {
            // Do not show while animated
            visible = false;
        } else if (mVisibleRect == q->boundingRect()) {
            // All of the image is visible
            visible = false;
        } else if (q->isUnderMouse() || !mStartDragMousePos.isNull()) {
            // User is interacting or about to interact with birdeyeview
            visible = true;
        } else if (mAutoHideTimer->isActive()) {
            // User triggered some activity recently (move mouse, scroll, zoom)
            visible = true;
        } else {
            // No recent activity
            visible = false;
        }
        const qreal wantedOpacity = visible ? 1 : 0;
        if (!qFuzzyCompare(wantedOpacity, q->opacity())) {
            mOpacityAnim->setEndValue(wantedOpacity);
            mOpacityAnim->start();
        }

        if (visible) {
            mAutoHideTimer->start();
        }
    }
};

BirdEyeView::BirdEyeView(DocumentView *docView)
    : QGraphicsWidget(docView)
    , d(new BirdEyeViewPrivate)
{
    d->q = this;
    d->mDocView = docView;
    setFlag(ItemIsSelectable);
    setCursor(Qt::ArrowCursor);
    setAcceptHoverEvents(true);

    d->mOpacityAnim = new QPropertyAnimation(this, "opacity", this);

    d->mAutoHideTimer = new QTimer(this);
    d->mAutoHideTimer->setSingleShot(true);
    d->mAutoHideTimer->setInterval(AUTOHIDE_DELAY);
    connect(d->mAutoHideTimer, &QTimer::timeout, this, &BirdEyeView::slotAutoHideTimeout);

    // Hide ourself by default, to avoid startup flashes (if we let updateOpacity
    // update opacity, it will do so through an animation)
    setOpacity(0);
    slotZoomOrSizeChanged();

    connect(docView->document().data(), &Document::metaInfoUpdated, this, &BirdEyeView::slotZoomOrSizeChanged);
    connect(docView, &DocumentView::zoomChanged, this, &BirdEyeView::slotZoomOrSizeChanged);
    connect(docView, &DocumentView::zoomToFitChanged, this, &BirdEyeView::slotZoomOrSizeChanged);
    connect(docView, &DocumentView::positionChanged, this, &BirdEyeView::slotPositionChanged);
}

BirdEyeView::~BirdEyeView()
{
    delete d;
}

void BirdEyeView::adjustGeometry()
{
    if (!d->mDocView->canZoom() || d->mDocView->zoomToFit()) {
        return;
    }
    QSizeF size = d->mDocView->document()->size() / qApp->devicePixelRatio();
    size.scale(MIN_SIZE, MIN_SIZE, Qt::KeepAspectRatioByExpanding);
    QRectF docViewRect = d->mDocView->boundingRect();
    int maxBevHeight = docViewRect.height() - 2 * VIEW_OFFSET;
    int maxBevWidth = docViewRect.width() - 2 * VIEW_OFFSET;
    if (size.height() > maxBevHeight) {
        size.scale(MIN_SIZE, maxBevHeight, Qt::KeepAspectRatio);
    }
    if (size.width() > maxBevWidth) {
        size.scale(maxBevWidth, MIN_SIZE, Qt::KeepAspectRatio);
    }
    const QRectF geom = QRectF(QApplication::isRightToLeft() ? docViewRect.left() + VIEW_OFFSET : docViewRect.right() - VIEW_OFFSET - size.width(),
                               docViewRect.bottom() - VIEW_OFFSET - size.height(),
                               size.width(),
                               size.height());
    setGeometry(alignedRectF(geom));
    adjustVisibleRect();
}

void BirdEyeView::adjustVisibleRect()
{
    const QSizeF docSize = d->mDocView->document()->size() / qApp->devicePixelRatio();
    const qreal viewZoom = d->mDocView->zoom();
    qreal bevZoom;
    if (docSize.height() > docSize.width()) {
        bevZoom = size().height() / docSize.height();
    } else {
        bevZoom = size().width() / docSize.width();
    }

    if (qFuzzyIsNull(viewZoom) || qFuzzyIsNull(bevZoom)) {
        // Prevent divide-by-zero crashes
        return;
    }

    const QRectF rect = QRectF(QPointF(d->mDocView->position()) / viewZoom * bevZoom, (d->mDocView->size() / viewZoom).boundedTo(docSize) * bevZoom);
    d->mVisibleRect = rect;
}

void BirdEyeView::slotAutoHideTimeout()
{
    d->updateVisibility();
}

void BirdEyeView::slotZoomOrSizeChanged()
{
    if (!d->mDocView->canZoom() || d->mDocView->zoomToFit()) {
        d->updateVisibility();
        return;
    }
    adjustGeometry();
    update();
    d->mAutoHideTimer->start();
    d->updateVisibility();
}

void BirdEyeView::slotPositionChanged()
{
    adjustVisibleRect();
    update();
    d->mAutoHideTimer->start();
    d->updateVisibility();
}

inline void drawTransparentRect(QPainter *painter, const QRectF &rect, const QColor &color)
{
    QColor bg = color;
    bg.setAlphaF(.33);
    QColor fg = color;
    fg.setAlphaF(.66);
    painter->setPen(fg);
    painter->setBrush(bg);
    painter->drawRect(rect.adjusted(0, 0, -1, -1));
}

void BirdEyeView::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static const QColor bgColor = QColor::fromHsvF(0, 0, .33);
    drawTransparentRect(painter, boundingRect(), bgColor);
    drawTransparentRect(painter, d->mVisibleRect, Qt::white);
}

void BirdEyeView::onMouseMoved()
{
    d->mAutoHideTimer->start();
    d->updateVisibility();
}

void BirdEyeView::slotIsAnimatedChanged()
{
    d->updateVisibility();
}

void BirdEyeView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (d->mVisibleRect.contains(event->pos()) && event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
        d->mStartDragMousePos = event->pos();
        d->mStartDragViewPos = d->mDocView->position();
    }
}

void BirdEyeView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if (d->mStartDragMousePos.isNull()) {
        // Do not drag if mouse was pressed outside visible rect
        return;
    }
    const qreal ratio = qMin(d->mDocView->boundingRect().height() / d->mVisibleRect.height(), d->mDocView->boundingRect().width() / d->mVisibleRect.width());
    const QPointF mousePos = event->pos();
    const QPointF viewPos = d->mStartDragViewPos + (mousePos - d->mStartDragMousePos) * ratio;

    d->mDocView->setPosition(viewPos.toPoint());
}

void BirdEyeView::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (d->mStartDragMousePos.isNull()) {
        return;
    }
    d->updateCursor(event->pos());
    d->mStartDragMousePos = QPointF();
    d->mAutoHideTimer->start();
}

void BirdEyeView::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    d->mAutoHideTimer->stop();
    d->updateVisibility();
}

void BirdEyeView::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (d->mStartDragMousePos.isNull()) {
        d->updateCursor(event->pos());
    }
}

void BirdEyeView::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    d->mAutoHideTimer->start();
    d->updateVisibility();
}

} // namespace
