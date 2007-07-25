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
#include "croptool.h"

// Qt
#include <QPainter>
#include <QScrollBar>
#include <QRect>

// KDE

// Local
#include "imageview.h"

namespace Gwenview {


struct CropToolPrivate {
	QRect mRect;
};


CropTool::CropTool(QObject* parent)
: AbstractImageViewTool(parent)
, d(new CropToolPrivate) {
}


CropTool::~CropTool() {
	delete d;
}


void CropTool::setRect(const QRect& rect) {
	d->mRect = rect;
	imageView()->viewport()->update();
}


void CropTool::paint(QPainter* painter) {
	QRect rect = imageView()->mapToViewport(d->mRect);
	/*
	qreal zoom = imageView()->zoom();
	QRect rect(
		int(d->mRect.left() * zoom),
		int(d->mRect.top() * zoom),
		int(d->mRect.width() * zoom),
		int(d->mRect.height() * zoom)
	);

	QPoint offset = imageView()->imageOffset();

	rect.moveLeft(rect.left() + offset.x() - imageView()->horizontalScrollBar()->value());

	rect.moveTop(rect.top() + offset.y() - imageView()->verticalScrollBar()->value());
	*/
	painter->drawRect(rect);
}

} // namespace
