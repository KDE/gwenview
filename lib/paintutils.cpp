/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "paintutils.h"

// STL
#include <cmath>

// Qt
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>

namespace Gwenview
{
namespace PaintUtils
{
// Copied from KFileItemDelegate
QPainterPath roundedRectangle(const QRectF &rect, qreal radius)
{
    QPainterPath path(QPointF(rect.left(), rect.top() + radius));
    path.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top()); // Top left corner
    path.lineTo(rect.right() - radius, rect.top()); // Top side
    path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius); // Top right corner
    path.lineTo(rect.right(), rect.bottom() - radius); // Right side
    path.quadTo(rect.right(), rect.bottom(), rect.right() - radius, rect.bottom()); // Bottom right corner
    path.lineTo(rect.left() + radius, rect.bottom()); // Bottom side
    path.quadTo(rect.left(), rect.bottom(), rect.left(), rect.bottom() - radius); // Bottom left corner
    path.closeSubpath();

    return path;
}

QPixmap generateFuzzyRect(const QSize &size, const QColor &color, int radius)
{
    QPixmap pix(size);
    const QColor transparent(0, 0, 0, 0);
    pix.fill(transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Fill middle
    painter.fillRect(pix.rect().adjusted(radius, radius, -radius, -radius), color);

    // Corners
    QRadialGradient gradient;
    gradient.setColorAt(0, color);
    gradient.setColorAt(1, transparent);
    gradient.setRadius(radius);
    QPoint center;

    // Top Left
    center = QPoint(radius, radius);
    gradient.setCenter(center);
    gradient.setFocalPoint(center);
    painter.fillRect(0, 0, radius, radius, gradient);

    // Top right
    center = QPoint(size.width() - radius, radius);
    gradient.setCenter(center);
    gradient.setFocalPoint(center);
    painter.fillRect(center.x(), 0, radius, radius, gradient);

    // Bottom left
    center = QPoint(radius, size.height() - radius);
    gradient.setCenter(center);
    gradient.setFocalPoint(center);
    painter.fillRect(0, center.y(), radius, radius, gradient);

    // Bottom right
    center = QPoint(size.width() - radius, size.height() - radius);
    gradient.setCenter(center);
    gradient.setFocalPoint(center);
    painter.fillRect(center.x(), center.y(), radius, radius, gradient);

    // Borders
    QLinearGradient linearGradient;
    linearGradient.setColorAt(0, color);
    linearGradient.setColorAt(1, transparent);

    // Top
    linearGradient.setStart(0, radius);
    linearGradient.setFinalStop(0, 0);
    painter.fillRect(radius, 0, size.width() - 2 * radius, radius, linearGradient);

    // Bottom
    linearGradient.setStart(0, size.height() - radius);
    linearGradient.setFinalStop(0, size.height());
    painter.fillRect(radius, int(linearGradient.start().y()), size.width() - 2 * radius, radius, linearGradient);

    // Left
    linearGradient.setStart(radius, 0);
    linearGradient.setFinalStop(0, 0);
    painter.fillRect(0, radius, radius, size.height() - 2 * radius, linearGradient);

    // Right
    linearGradient.setStart(size.width() - radius, 0);
    linearGradient.setFinalStop(size.width(), 0);
    painter.fillRect(int(linearGradient.start().x()), radius, radius, size.height() - 2 * radius, linearGradient);
    return pix;
}

QColor adjustedHsv(const QColor &color, int deltaH, int deltaS, int deltaV)
{
    int hue, saturation, value;
    color.getHsv(&hue, &saturation, &value);
    return QColor::fromHsv(qBound(0, hue + deltaH, 359), qBound(0, saturation + deltaS, 255), qBound(0, value + deltaV, 255));
}

QColor alphaAdjustedF(const QColor &color, qreal alphaF)
{
    QColor tmp = color;
    tmp.setAlphaF(alphaF);
    return tmp;
}

} // namespace
} // namespace
