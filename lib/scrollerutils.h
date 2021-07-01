/*
Gwenview: an image viewer
Copyright 2019 Steffen Hartleib <steffenhartleib@t-online.de>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef SCROLLERUTILS_H
#define SCROLLERUTILS_H

#include <lib/gwenviewlib_export.h>

class QScroller;
class QObject;

namespace Gwenview
{
namespace ScrollerUtils
{
GWENVIEWLIB_EXPORT QScroller *setQScroller(QObject *);

} // namespace
} // namespace

#endif /* SCROLLERUTILS_H */
