// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2018 Nate Graham <pointedstick@zoho.com>

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
#ifndef RENDERINGINTENT_H
#define RENDERINGINTENT_H

#include <lcms2.h>

namespace Gwenview
{
namespace RenderingIntent
{
/**
 * This enum represents the different color rendering modes
 */
enum Enum : cmsUInt32Number {
    Perceptual = INTENT_PERCEPTUAL,
    Relative = INTENT_RELATIVE_COLORIMETRIC,
};

} // namespace RenderingIntent

} // namespace Gwenview

#endif /* RENDERINGINTENT_H */
