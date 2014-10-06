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
#ifndef PAINTUTILS_H
#define PAINTUTILS_H

#include <lib/gwenviewlib_export.h>
#include <QtGlobal>
class QColor;
class QPainterPath;
class QPixmap;
class QRect;
class QRectF;
class QSize;

namespace Gwenview
{

/**
 * A collection of independent painting functions
 */
namespace PaintUtils
{

/**
 * Returns a rounded-corner version of @rect. Corner radius is @p radius.
 * (Copied from KFileItemDelegate)
 */
GWENVIEWLIB_EXPORT QPainterPath roundedRectangle(const QRectF& rect, qreal radius);

/**
 * Generates a pixmap of size @p size, filled with @p color, whose borders have
 * been blurred by @p radius pixels.
 */
GWENVIEWLIB_EXPORT QPixmap generateFuzzyRect(const QSize& size, const QColor& color, int radius);

/**
 * Returns a modified version of @p color, where hue, saturation and value have
 * been adjusted according to @p deltaH, @p deltaS and @p deltaV.
 */
GWENVIEWLIB_EXPORT QColor adjustedHsv(const QColor& color, int deltaH, int deltaS, int deltaV);

/**
 * Returns a modified version of @p color, where alpha has been set to @p
 * alphaF.
 */
GWENVIEWLIB_EXPORT QColor alphaAdjustedF(const QColor& color, qreal alphaF);

/**
 * Returns the smallest QRect which contains @p rectF
 */
GWENVIEWLIB_EXPORT QRect containingRect(const QRectF& rectF);

} // namespace

} // namespace

#endif /* PAINTUTILS_H */
