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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "document.moc"

// Qt
#include <QImage>

namespace Gwenview {


struct Document::Private {
	QImage mImage;
};


Document::Document() 
: QObject() {
	d.reset(new Document::Private);
}


void Document::load(const QString& path) {
	d->mImage.load(path);
	loaded();
}


QImage& Document::image() {
	return d->mImage;
}

} // namespace
