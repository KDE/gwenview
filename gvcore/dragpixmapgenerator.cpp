// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurélien Gâteau

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
#include "dragpixmapgenerator.h"

// Qt
#include <qimage.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qtooltip.h>

// KDE
#include <kdebug.h>

namespace Gwenview {


static const int DRAG_SPACING=6;
static const int DRAG_MAX_PIX_SIZE=64;


inline QPixmap reducePixmap(const QPixmap& src, int size) {
	if (src.width() <= size && src.height() <= size) return src;
	QImage img = src.convertToImage();
	img = img.smoothScale(size, size, QImage::ScaleMin);
	return QPixmap(img);
}


QPixmap dragPixmapGeneratorHelper(QValueVector<QPixmap> pixmapVector) {
	QPixmap dragPixmap;
	int dragCount = pixmapVector.count();

	// Compute width and height of pixmap
	int width = 0;
	int height = DRAG_SPACING;
	for (int pos=0; pos < dragCount; ++pos) {
		QPixmap pix = pixmapVector[pos];
		pix = reducePixmap(pix, DRAG_MAX_PIX_SIZE);
		pixmapVector[pos] = pix;

		width = QMAX(width, pix.width());
		height += (pix.height() + DRAG_SPACING);
	}
	width += 2 * DRAG_SPACING;

	// Create pixmap
	QPalette palette = QToolTip::palette();
	
	dragPixmap = QPixmap(width, height);
	dragPixmap.fill(palette.active().base());
	
	QPainter painter(&dragPixmap);
	painter.setPen(palette.active().dark());
	painter.drawRect(dragPixmap.rect());

	// Draw item pixmaps in it
	int yOffset = DRAG_SPACING;
	for (int pos=0; pos < dragCount; ++pos) {
		QPixmap pix = pixmapVector[pos];
		int xOffset = (width - pix.width()) / 2;
		
		painter.drawPixmap(xOffset, yOffset, pix);
		yOffset += pix.height() + DRAG_SPACING;
	}
	return dragPixmap;
}


} // namespace
