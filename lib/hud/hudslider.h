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
#ifndef HUDSLIDER_H
#define HUDSLIDER_H

// Local

// KF

// Qt
#include <QAbstractSlider>
#include <QGraphicsWidget>

namespace Gwenview
{
struct HudSliderPrivate;
/**
 * A QGraphicsView slider.
 * Provides more-or-less the same API as QSlider.
 */
class HudSlider : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit HudSlider(QGraphicsItem *parent = nullptr);
    ~HudSlider() override;

    void setRange(int min, int max);
    void setPageStep(int step);
    void setSingleStep(int step);
    void setValue(int value);

    int sliderPosition() const;
    void setSliderPosition(int);

    bool isSliderDown() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    void triggerAction(QAbstractSlider::SliderAction);

Q_SIGNALS:
    void valueChanged(int);
    void actionTriggered(int);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private Q_SLOTS:
    void doRepeatAction(int time = 50);

private:
    HudSliderPrivate *const d;
};

} // namespace

#endif /* HUDSLIDER_H */
