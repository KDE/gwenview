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

/*
   *****************************************************
   ******************************************************
   **** NOTE: This class is deprecated. Do not use it. **
   ****   It will be removed in the QT 6 timeframe.    **
   ******************************************************
   ******************************************************
*/


// Self
#include "contextbarbutton.h"

// Local
#include "paintutils.h"

// KDE
#include <KIconLoader>

// Qt
#include <QStyleOptionToolButton>
#include <QStylePainter>

namespace Gwenview
{

/** How lighter is the border of context bar buttons */
const int CONTEXTBAR_BORDER_LIGHTNESS = 140;

/** How darker is the background of context bar buttons */
const int CONTEXTBAR_BACKGROUND_DARKNESS = 170;

/** How lighter are context bar buttons when under mouse */
const int CONTEXTBAR_MOUSEOVER_LIGHTNESS = 115;

/** Radius of ContextBarButtons */
const int CONTEXTBAR_RADIUS = 5;

struct ContextBarButtonPrivate
{
};

ContextBarButton::ContextBarButton(const QString& iconName, QWidget* parent)
: QToolButton(parent)
, d(new ContextBarButtonPrivate)
{
    const int size = KIconLoader::global()->currentSize(KIconLoader::Small);
    setIconSize(QSize(size, size));
    setAutoRaise(true);
    setIcon(SmallIcon(iconName));
}

ContextBarButton::~ContextBarButton()
{
    delete d;
}

void Gwenview::ContextBarButton::paintEvent(QPaintEvent*)
{
    QStylePainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    const QColor bgColor = palette().color(backgroundRole());
    QColor color = bgColor.darker(CONTEXTBAR_BACKGROUND_DARKNESS);
    QColor borderColor = bgColor.lighter(CONTEXTBAR_BORDER_LIGHTNESS);

    if (opt.state & QStyle::State_MouseOver && opt.state & QStyle::State_Enabled) {
        color = color.lighter(CONTEXTBAR_MOUSEOVER_LIGHTNESS);
        borderColor = borderColor.lighter(CONTEXTBAR_MOUSEOVER_LIGHTNESS);
    }

    const QRectF rectF = QRectF(opt.rect).adjusted(0.5, 0.5, -0.5, -0.5);
    const QPainterPath path = PaintUtils::roundedRectangle(rectF, CONTEXTBAR_RADIUS);

    // Background
    painter.fillPath(path, color);

    // Top shadow
    QLinearGradient gradient(rectF.topLeft(), rectF.topLeft() + QPoint(0, 5));
    gradient.setColorAt(0, QColor::fromHsvF(0, 0, 0, .3));
    gradient.setColorAt(1, Qt::transparent);
    painter.fillPath(path, gradient);

    // Left shadow
    gradient.setFinalStop(rectF.topLeft() + QPoint(5, 0));
    painter.fillPath(path, gradient);

    // Border
    painter.setPen(borderColor);
    painter.drawPath(path);

    // Content
    painter.drawControl(QStyle::CE_ToolButtonLabel, opt);
}

} // namespace
