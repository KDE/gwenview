/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>
Copyright 2018 Huon Imberger <huon@plonq.org>

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

#include "stylesheetutils.h"

// Qt

// KDE

// Local
#include "paintutils.h"

namespace Gwenview
{

namespace StyleSheetUtils
{

QString rgba(const QColor &color)
{
    return QString::fromLocal8Bit("rgba(%1, %2, %3, %4)")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alpha());
}

QString gradient(Qt::Orientation orientation, const QColor &color, int value)
{
    int x2, y2;
    if (orientation == Qt::Horizontal) {
        x2 = 0;
        y2 = 1;
    } else {
        x2 = 1;
        y2 = 0;
    }
    QString grad =
        QStringLiteral("qlineargradient(x1:0, y1:0, x2:%1, y2:%2,"
        "stop:0 %3, stop: 1 %4)");
    return grad
            .arg(x2)
            .arg(y2)
            .arg(rgba(PaintUtils::adjustedHsv(color, 0, 0, qMin(255 - color.value(), value / 2))))
            .arg(rgba(PaintUtils::adjustedHsv(color, 0, 0, -qMin(color.value(), value / 2))))
            ;
}

} // namespace

} // namespace
