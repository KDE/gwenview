// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef THUMBNAILGROUP_H
#define THUMBNAILGROUP_H

// Qt

// KDE

// Local

namespace Gwenview
{

namespace ThumbnailGroup
{
enum Enum {
    Normal,
    Large,
    Large2x
};

inline int pixelSize(const Enum value)
{
    switch(value) {
    case Normal:
        return 128;
    case Large:
        return 256;
    case Large2x:
        return 512;
    default:
        return 128;
    }
}

inline Enum fromPixelSize(int value)
{
    if (value <= 128) {
        return Normal;
    } else if (value <= 256) {
        return Large;
    } else {
        return Large2x;
    }
}
} // namespace ThumbnailGroup

} // namespace Gwenview

#endif /* THUMBNAILGROUP_H */
