/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef TESTUTILS_H
#define TESTUTILS_H

// Qt
#include <QDir>

// KDE
#include <kurl.h>


/*
 * This file contains simple helpers to access test files
 */

inline QString pathForTestFile(const QString& name) {
	return QString("%1/%2").arg(KDESRCDIR).arg(name);
}

inline KUrl urlForTestFile(const QString& name) {
	KUrl url;
	url.setPath(pathForTestFile(name));
	return url;
}

inline QString pathForTestOutputFile(const QString& name) {
	return QString("%1/%2").arg(QDir::currentPath()).arg(name);
}

inline KUrl urlForTestOutputFile(const QString& name) {
	KUrl url;
	url.setPath(pathForTestOutputFile(name));
	return url;
}

#endif /* TESTUTILS_H */
