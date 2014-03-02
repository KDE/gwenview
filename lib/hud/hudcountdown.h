// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2014 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef HUDCOUNTDOWN_H
#define HUDCOUNTDOWN_H

// Local

// KDE

// Qt
#include <QGraphicsWidget>

namespace Gwenview
{

struct HudCountDownPrivate;

/**
 * Displays a count-down pie-chart
 */
class HudCountDown : public QGraphicsWidget
{
    Q_OBJECT
public:
    HudCountDown(QGraphicsWidget* parent = 0);
    ~HudCountDown();

    void start(qreal ms);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

Q_SIGNALS:
    void timeout();

private Q_SLOTS:
    void doUpdate();

private:
    HudCountDownPrivate* const d;
};

} // namespace

#endif /* HUDCOUNTDOWN_H */
