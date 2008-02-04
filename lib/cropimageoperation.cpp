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
// Self
#include "cropimageoperation.h"

// Qt
#include <QImage>
#include <QPainter>

// KDE
#include <klocale.h>

// Local
#include "document/document.h"

namespace Gwenview {


struct CropImageOperationPrivate {
	QRect mRect;
	QImage mOriginalImage;
};


CropImageOperation::CropImageOperation(const QRect& rect)
: d(new CropImageOperationPrivate) {
	d->mRect = rect;
	setText(i18n("Crop"));
}


CropImageOperation::~CropImageOperation() {
	delete d;
}


void CropImageOperation::redo() {
	QImage src = document()->image();
	d->mOriginalImage = src;
	QImage dst(d->mRect.size(), src.format());
	QPainter painter(&dst);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawImage(QPoint(0, 0), src, d->mRect);
	painter.end();
	document()->setImage(dst);
}


void CropImageOperation::undo() {
	document()->setImage(d->mOriginalImage);
}


} // namespace
