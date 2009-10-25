// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "renamer.h"

// Qt
#include <QFileInfo>

// KDE
#include <kdatetime.h>
#include <kurl.h>

// Local

namespace Gwenview {


struct RenamerPrivate {
	QString mFormat;
};


Renamer::Renamer(const QString& format)
: d(new RenamerPrivate) {
	d->mFormat = format;
}


Renamer::~Renamer() {
	delete d;
}


QString Renamer::rename(const KUrl& url, const KDateTime& dateTime) {
	QFileInfo info(url.fileName());
	return dateTime.toString(d->mFormat) + '.' + info.completeSuffix();
}


} // namespace
