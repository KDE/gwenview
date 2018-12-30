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
#ifndef EXIV2IMAGELOADER_H
#define EXIV2IMAGELOADER_H

#include <lib/gwenviewlib_export.h>

// STL
#include <memory>

// Qt

// KDE

// Exiv2
namespace Exiv2
{
    class Image;
}

// Local

class QByteArray;
class QString;

namespace Gwenview
{

struct Exiv2ImageLoaderPrivate;

/**
 * This helper class loads image using libexiv2, and takes care of exception
 * handling for the different versions of libexiv2.
 */
class GWENVIEWLIB_EXPORT Exiv2ImageLoader
{
public:
    Exiv2ImageLoader();
    ~Exiv2ImageLoader();

    bool load(const QString&);
    bool load(const QByteArray&);
    QString errorMessage() const;
    std::unique_ptr<Exiv2::Image> popImage();

private:
    Exiv2ImageLoaderPrivate* const d;
};

} // namespace

#endif /* EXIV2IMAGELOADER_H */
