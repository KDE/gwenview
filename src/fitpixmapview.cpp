/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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
// QT includes
#include <qpixmap.h>
#include <qpainter.h>
#include <qstyle.h>

// KDE includes
#include <kconfig.h>
#include <kdebug.h>

// Our includes
#include "gvpixmap.h"

#include "fitpixmapview.moc"


FitPixmapView::FitPixmapView(QWidget* parent, GVPixmap* pixmap,bool enabled)
: QFrame(parent,0L,WPaintClever | WResizeNoErase | WRepaintNoErase), mGVPixmap(pixmap)
{
	setFocusPolicy(StrongFocus);
	setFullScreen(false);
	enableView(enabled);
}


void FitPixmapView::enableView(bool enabled) {
	disconnect(mGVPixmap,0,this,0);

	if (enabled) {
		connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
			this,SLOT(updateView()) );
		setFocus();
	}
}


void FitPixmapView::updateView() {
	updateZoomedPixmap();
	repaint();
}


void FitPixmapView::setFullScreen(bool fullScreen) {
	if (fullScreen) {
		setBackgroundColor(black);
		setFrameStyle(NoFrame);
	} else {
		setBackgroundMode(PaletteDark);
		setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
		setLineWidth( style().defaultFrameWidth() );
	}
}


//-Overloaded functions-------------------
void FitPixmapView::drawContents(QPainter* painter) {
	if (!mGVPixmap->isNull()) {
		paintPixmap(painter);
	} else {
		painter->eraseRect(contentsRect());
	}
	emit zoomChanged(mZoom);
}


void FitPixmapView::resizeEvent(QResizeEvent*) {
	updateZoomedPixmap();
	repaint();
}


//-Private--------------------------------
void FitPixmapView::paintPixmap(QPainter* painter) {
	QRect clip=painter->clipRegion().boundingRect();
	int pixmapWidth,pixmapHeight,contentsWidth,contentsHeight;
	int xOffset,yOffset;

// compute vars
	pixmapWidth=mZoomedPixmap.width();
	pixmapHeight=mZoomedPixmap.height();
	contentsWidth=contentsRect().width();
	contentsHeight=contentsRect().height();
	xOffset=int(contentsWidth-pixmapWidth)/2;
	yOffset=int(contentsHeight-pixmapHeight)/2;

// Paint image
	painter->drawPixmap(
		clip.left(),clip.top(),
		mZoomedPixmap,
		clip.left()-xOffset,clip.top()-yOffset,
		clip.width(),clip.height());
}


void FitPixmapView::updateZoomedPixmap() {
	int srcWidth,srcHeight,dstWidth,dstHeight;
	srcWidth=mGVPixmap->width();
	srcHeight=mGVPixmap->height();
	dstWidth=contentsRect().width();
	dstHeight=contentsRect().height();

// If no need to zoom, copy the pixmap and get out
	if (srcWidth<=dstWidth && srcHeight<=dstHeight) {
		mZoomedPixmap=mGVPixmap->pixmap();
		mZoom=1.0;
		return;
	}
	
// Compute zoom
	double widthZoom=double(dstWidth) / double(srcWidth);
	double heightZoom=double(dstHeight) / double(srcHeight);

	mZoom=QMIN( heightZoom,widthZoom);

// Zoom pixmap
	mZoomedPixmap.resize(int(srcWidth*mZoom),int(srcHeight*mZoom));
	QPainter painter(&mZoomedPixmap);
	painter.scale(mZoom,mZoom);
	painter.drawPixmap(0,0,mGVPixmap->pixmap());
}


//-Configuration--------------------------
void FitPixmapView::readConfig(KConfig*,const QString&) {
}


void FitPixmapView::writeConfig(KConfig*,const QString&) const {
}
