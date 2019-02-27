// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#include "imageutils.h"

// Qt
#include <QTransform>

namespace Gwenview
{
namespace ImageUtils
{

QTransform transformMatrix(Orientation orientation)
{
    QTransform matrix;
    switch (orientation) {
    case NOT_AVAILABLE:
    case NORMAL:
        break;

    case HFLIP:
        matrix.scale(-1, 1);
        break;

    case ROT_180:
        matrix.rotate(180);
        break;

    case VFLIP:
        matrix.scale(1, -1);
        break;

    case TRANSPOSE:
        matrix.scale(-1, 1);
        matrix.rotate(90);
        break;

    case ROT_90:
        matrix.rotate(90);
        break;

    case TRANSVERSE:
        matrix.scale(1, -1);
        matrix.rotate(90);
        break;

    case ROT_270:
        matrix.rotate(270);
        break;
    }

    return matrix;
}

} // namespace
} // namespace
