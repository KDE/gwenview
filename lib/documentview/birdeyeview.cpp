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
#include "birdeyeview.moc"

// Local
#include <lib/document/document.h>
#include <lib/documentview/documentview.h>

// KDE
#include <KDebug>

// Qt
#include <QCursor>
#include <QGraphicsSceneEvent>
#include <QPainter>

namespace Gwenview
{

static qreal Y_POSITION_PERCENT = 1 / 3.;
static qreal MIN_SIZE = 72;
static qreal VIEW_OFFSET = MIN_SIZE / 4;

struct BirdEyeViewPrivate
{
    BirdEyeView* q;
    DocumentView* mDocView;
    QRectF mVisibleRect;
    QPointF mLastDragPos;

    void updateCursor(const QPointF& pos)
    {
        q->setCursor(mVisibleRect.contains(pos) ? Qt::OpenHandCursor : Qt::ArrowCursor);
    }
};

BirdEyeView::BirdEyeView(DocumentView* docView)
: QGraphicsWidget(docView)
, d(new BirdEyeViewPrivate)
{
    d->q = this;
    d->mDocView = docView;
    setFlag(ItemIsSelectable);
    setCursor(Qt::ArrowCursor);
    setAcceptHoverEvents(true);
    adjustGeometry();

    connect(docView->document().data(), SIGNAL(metaInfoUpdated()), SLOT(adjustGeometry()));
    connect(docView, SIGNAL(zoomChanged(qreal)), SLOT(adjustGeometry()));
    connect(docView, SIGNAL(zoomToFitChanged(bool)), SLOT(adjustGeometry()));
    connect(docView, SIGNAL(positionChanged()), SLOT(adjustVisibleRect()));
}

BirdEyeView::~BirdEyeView()
{
    delete d;
}

void BirdEyeView::adjustGeometry()
{
    if (!d->mDocView->canZoom() || d->mDocView->zoomToFit()) {
        hide();
        return;
    }
    show();
    QSize size = d->mDocView->document()->size();
    size.scale(MIN_SIZE, MIN_SIZE, Qt::KeepAspectRatioByExpanding);
    QRectF rect = d->mDocView->boundingRect();
    setGeometry(
        QRectF(
            rect.right() - VIEW_OFFSET - size.width(),
            qMax(rect.top() + rect.height() * Y_POSITION_PERCENT - size.height(), qreal(0.)),
            size.width(),
            size.height()
        ));
    adjustVisibleRect();
    setVisible(d->mVisibleRect != boundingRect());
}

void BirdEyeView::adjustVisibleRect()
{
    QSizeF docSize = d->mDocView->document()->size();
    qreal viewZoom = d->mDocView->zoom();
    qreal bevZoom = size().width() / docSize.width();
    if (qFuzzyIsNull(viewZoom) || qFuzzyIsNull(bevZoom)) {
        // Prevent divide-by-zero crashes
        return;
    }

    d->mVisibleRect = QRectF(
                          QPointF(d->mDocView->position()) / viewZoom * bevZoom,
                          (d->mDocView->size() / viewZoom).boundedTo(docSize) * bevZoom);
    update();
}

inline void drawTransparentRect(QPainter* painter, const QRectF& rect, const QColor& color)
{
    QColor bg = color;
    bg.setAlphaF(.33);
    QColor fg = color;
    fg.setAlphaF(.66);
    painter->setPen(fg);
    painter->setBrush(bg);
    // Use a QRect to avoid missing pixels in the corners
    painter->drawRect(rect.toRect().adjusted(0, 0, -1, -1));
}

void BirdEyeView::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    static const QColor bgColor = QColor::fromHsvF(0, 0, .33);
    drawTransparentRect(painter, boundingRect(), bgColor);
    drawTransparentRect(painter, d->mVisibleRect, Qt::white);
}

void BirdEyeView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (d->mVisibleRect.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        d->mLastDragPos = event->pos();
    }
}

void BirdEyeView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if (d->mLastDragPos.isNull()) {
        // Do not drag if mouse was pressed outside visible rect
        return;
    }
    qreal ratio = d->mDocView->boundingRect().width() / d->mVisibleRect.width();

    QPointF mousePos = event->pos();
    QPointF viewPos = d->mDocView->position() + (mousePos - d->mLastDragPos) * ratio;

    d->mLastDragPos = mousePos;
    d->mDocView->setPosition(viewPos.toPoint());
}

void BirdEyeView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (d->mLastDragPos.isNull()) {
        return;
    }
    d->updateCursor(event->pos());
    d->mLastDragPos = QPointF();
}

void BirdEyeView::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (d->mLastDragPos.isNull()) {
        d->updateCursor(event->pos());
    }
}

} // namespace
