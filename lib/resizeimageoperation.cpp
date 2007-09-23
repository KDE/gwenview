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
// Self
#include "resizeimageoperation.h"

// Qt
#include <QImage>

// KDE

// Local
#include "document/document.h"

namespace Gwenview {


struct ResizeImageOperationPrivate {
	int mSize;
};


ResizeImageOperation::ResizeImageOperation(int size)
: d(new ResizeImageOperationPrivate) {
	d->mSize = size;
}


ResizeImageOperation::~ResizeImageOperation() {
	delete d;
}


void ResizeImageOperation::apply(Document::Ptr doc) {
	QImage image = doc->image();
	image = image.scaled(d->mSize, d->mSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	doc->setImage(image);
}


} // namespace
