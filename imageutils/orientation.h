// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
 
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
#ifndef ORIENTATION_H
#define ORIENTATION_H


namespace ImageUtils {

/* Explanation extracted from http://sylvana.net/jpegcrop/exif_orientation.html

   For convenience, here is what the letter F would look like if it were tagged
correctly and displayed by a program that ignores the orientation tag (thus
showing the stored image): 

  1        2       3      4         5            6           7          8

888888  888888      88  88      8888888888  88                  88  8888888888
88          88      88  88      88  88      88  88          88  88      88  88
8888      8888    8888  8888    88          8888888888  8888888888          88
88          88      88  88
88          88  888888  888888

*/

enum Orientation {
	NOT_AVAILABLE=0,
	NORMAL=1,
	HFLIP=2,
	ROT_180=3,
	VFLIP=4,
	TRANSPOSE=5,
	ROT_90=6,
	TRANSVERSE=7,
	ROT_270=8
};

}


#endif
