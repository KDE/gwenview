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
#ifndef BIRDEYEVIEW_H
#define BIRDEYEVIEW_H

// Local

// KF

// Qt
#include <QGraphicsWidget>

namespace Gwenview
{

class DocumentView;

struct BirdEyeViewPrivate;
/**
 * Shows a bird-eye view of the current document. Makes it possible to scroll
 * through the document.
 */
class BirdEyeView : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit BirdEyeView(DocumentView* docView);
    ~BirdEyeView() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    // Called by DocumentView when it detects mouse movements
    // We cannot use a sceneEventFilter because QGraphicsSceneHoverEvent are not
    // sent to parent items (unlike QHoverEvent). Therefore DocumentView has to
    // do the work of event-filtering the actual document widget.
    void onMouseMoved();

public Q_SLOTS:
    void slotZoomOrSizeChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private Q_SLOTS:
    void slotAutoHideTimeout();
    void slotPositionChanged();
    void slotIsAnimatedChanged();

private:
    BirdEyeViewPrivate* const d;
    void adjustVisibleRect();
    void adjustGeometry();
};

} // namespace

#endif /* BIRDEYEVIEW_H */
