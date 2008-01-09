// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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

// Qt

// KDE

// Exiv2
#include <exiv2/image.hpp>

// Local

class QByteArray;
class QString;

namespace Gwenview {


class Exiv2ImageLoaderPrivate;

/**
 * This helper class loads image using libexiv2, and takes care of exception
 * handling for the different versions of libexiv2.
 */
class Exiv2ImageLoader {
public:
	Exiv2ImageLoader();
	~Exiv2ImageLoader();

	bool load(const QByteArray&);
	QString errorMessage() const;
	Exiv2::Image::AutoPtr popImage();

private:
	Exiv2ImageLoaderPrivate* const d;
};


} // namespace

#endif /* EXIV2IMAGELOADER_H */
