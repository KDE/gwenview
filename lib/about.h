// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2013 Aurélien Gâteau <agateau@kde.org>

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
#ifndef ABOUT_H
#define ABOUT_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt

class KAboutData;
class QByteArray;
class KLocalizedString;

namespace Gwenview
{

GWENVIEWLIB_EXPORT KAboutData* createAboutData(const QByteArray& appName, const QByteArray& catalogName, const KLocalizedString& programName);

} // namespace

#endif /* ABOUT_H */
