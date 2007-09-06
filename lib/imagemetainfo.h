// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef IMAGEMETAINFO_H
#define IMAGEMETAINFO_H

#include "gwenviewlib_export.h"

// Qt

// KDE

// Local

class KFileItem;

namespace Exiv2 { class Image; }

namespace Gwenview {


class ImageMetaInfoPrivate;
class GWENVIEWLIB_EXPORT ImageMetaInfo {
public:
	ImageMetaInfo();
	~ImageMetaInfo();

	void setFileItem(const KFileItem&);
	void setExiv2Image(const Exiv2::Image*);

	void getInfoForKey(const QString& key, QString* label, QString* value) const;

private:
	ImageMetaInfoPrivate* const d;
};


} // namespace

#endif /* IMAGEMETAINFO_H */
