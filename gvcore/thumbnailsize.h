// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*	Gwenview - A simple image viewer for KDE
	Copyright 2000-2004 Aurélien Gâteau
	This class is based on the ImagePreviewJob class from Konqueror.
	Original copyright follows.
*/
/*	This file is part of the KDE project
	Copyright (C) 2000 David Faure <faure@kde.org>
				  2000 Carsten Pfeiffer <pfeiffer@kde.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

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

namespace Gwenview {

/**
 * This namespace stores constants used by several files. It avoids multiple
 * includes.
 */
namespace ThumbnailSize {
	const int MIN=48;
	const int NORMAL=128;
	const int LARGE=256;
}

} // namespace
#endif /* THUMBNAILSIZE_H */

