// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010-2013 Aurélien Gâteau <agateau@kde.org>

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
#ifndef VERSION_H
#define VERSION_H

/*

For stable releases, GWENVIEW_VERSION should be "$major.$minor.$patch",
matching KDE SC versions.

For unstable releases, it should be "$major.$minor.$patch $suffix", where
suffix is one of "pre", "alpha$N", "beta$N" or "rc$N".

When you change GWENVIEW_VERSION, add the new version in Bugzilla as well:
https://bugs.kde.org/editversions.cgi?product=gwenview

*/
#define GWENVIEW_VERSION "4.12.2"

#endif /* VERSION_H */
