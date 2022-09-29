/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurlien Gteau
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QImage>

namespace Gwenview
{
namespace ImageUtils
{
QImage changeBrightness(const QImage &image, int brightness);
QImage changeContrast(const QImage &image, int contrast);
QImage changeGamma(const QImage &image, int gamma);
}

}
#endif
