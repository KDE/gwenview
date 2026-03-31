// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2026 Robert Siebeck <robert@siebeck.org>

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
#ifndef MOTIONPHOTOUTIL_H
#define MOTIONPHOTOUTIL_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QByteArray>

class QString;

namespace Gwenview
{
namespace MotionPhotoUtil
{

/**
 * Returns true if the file at @p path is a Google Pixel motion photo,
 * detected via XMP GCamera metadata.
 */
GWENVIEWLIB_EXPORT bool isMotionPhoto(const QString &path);

/**
 * Extracts the embedded MP4 video from a motion photo at @p path.
 * Returns an empty QByteArray if extraction fails.
 */
GWENVIEWLIB_EXPORT QByteArray extractVideo(const QString &path);

} // namespace MotionPhotoUtil
} // namespace Gwenview

#endif /* MOTIONPHOTOUTIL_H */
