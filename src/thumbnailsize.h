// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau

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
#ifndef THUMBNAILSIZE_H
#define THUMBNAILSIZE_H

// Qt includes
#include <qstring.h>

/**
 * This is an "enum-like" class : an enum with cast operators to convert
 * from/to string and a few other methods
 */
class ThumbnailSize {
public:
	enum Size { Small, Med, Large };
	ThumbnailSize(Size value=Med);

	/**
	 * Init a ThumbnailSize from a string, set to medium if the string
	 * is not recognized
	 */
	ThumbnailSize(const QString& str);

	int pixelSize() const;
	bool operator==(const ThumbnailSize& size) const { return mValue==size.mValue; }
	operator const QString&() const;
	operator Size() const { return mValue; }

	static ThumbnailSize biggest() { return ThumbnailSize(Large); }

private:
	Size mValue;
};

#endif
