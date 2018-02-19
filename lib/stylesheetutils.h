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
#ifndef STYLESHEETUTILS_H
#define STYLESHEETUTILS_H

// Qt
#include <QString>
#include <QColor>

// KDE

// Local
#include <lib/gwenviewlib_export.h>
#include "orientation.h"


namespace Gwenview
{

/**
 * A collection of convenience functions to generate CSS code
 */
namespace StyleSheetUtils
{

GWENVIEWLIB_EXPORT QString rgba(const QColor &color);

GWENVIEWLIB_EXPORT QString gradient(Qt::Orientation orientation, const QColor &color, int value);

} // namespace

} // namespace

#endif /* STYLESHEETUTILS_H */
