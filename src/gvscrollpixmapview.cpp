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

#include <math.h>

// Qt includes
#include <qcursor.h>
#include <qevent.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>

// KDE includes
#include <kconfig.h>
#include <kdebug.h>

// Our includes
#include "gvpixmap.h"

#include "gvscrollpixmapview.moc"

const double MAX_ZOOM=16.0; // Same value as GIMP

GVScrollPixmapView::GVScrollPixmapView(QWidget* parent,GVPixmap* pixmap,bool enabled)
: QScrollView(parent,0L,WResizeNoErase|WRepaintNoErase)
, mGVPixmap(pixmap)
, mPopupMenu(0L)
, mZoom(1)
, mLockZoom(false)
, mDragStarted(false)
{
	unsetCursor();
	setFocusPolicy(StrongFocus);
	setFullScreen(false);
	enableView(enabled);
}


void GVScrollPixmapView::enableView(bool enabled) {
	disconnect(mGVPixmap,0,this,0);

	if (enabled) {
		connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
			this,SLOT(updateView()) );
	}
}


void GVScrollPixmapView::updateView() {
	mXOffset=0;
	mYOffset=0;
	if (mGVPixmap->isNull()) {
		resizeContents(0,0);
		viewport()->repaint(false);
		return;
	}

	if (!mLockZoom) {
		setZoom(1.0);
	} else {
		updateContentSize();
		updateImageOffset(mZoom);
		viewport()->repaint(false);
	}
	horizontalScrollBar()->setValue(0);
	verticalScrollBar()->setValue(0);
}


void GVScrollPixmapView::setZoom(double zoom) {
	double oldZoom=mZoom;
	mZoom=zoom;
	updateContentSize();
	updateImageOffset(oldZoom);
	viewport()->repaint(false);
	emit zoomChanged(mZoom);
}


void GVScrollPixmapView::setFullScreen(bool fullScreen) {
	if (fullScreen) {
		viewport()->setBackgroundColor(black);
		setFrameStyle(NoFrame);
	} else {
		viewport()->setBackgroundMode(PaletteDark);
		setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
		setLineWidth( style().defaultFrameWidth() );
	}
}


bool GVScrollPixmapView::isZoomSetToMax() {
	return mZoom>=MAX_ZOOM;
}


bool GVScrollPixmapView::isZoomSetToMin() {
	return mZoom<=1/MAX_ZOOM;
}


//-Overloaded methods----------------------------------
void GVScrollPixmapView::resizeEvent(QResizeEvent* event) {
	QScrollView::resizeEvent(event);
	updateContentSize();
	updateImageOffset(mZoom);
}



void GVScrollPixmapView::drawContents(QPainter* painter,int clipx,int clipy,int clipw,int cliph) {
/*
	static QColor colors[4]={QColor(255,0,0),QColor(0,255,0),QColor(0,0,255),QColor(255,255,0) };
	static int numColor=0;*/ // DEBUG rects

	if (mGVPixmap->isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	int roundedClipx=int( int (  clipx        /mZoom)*mZoom );
	int roundedClipy=int( int (  clipy        /mZoom)*mZoom );
	int roundedClipw=int( ceil( (clipx+clipw) /mZoom)*mZoom ) - roundedClipx;
	int roundedCliph=int( ceil( (clipy+cliph) /mZoom)*mZoom ) - roundedClipy;

	if (roundedClipw<=0 || roundedCliph<=0) return;

	QPixmap buffer(roundedClipw,roundedCliph);
	{
		QPainter bufferPainter(&buffer);
		bufferPainter.setBackgroundColor(painter->backgroundColor());
		bufferPainter.eraseRect(0,0,roundedClipw,roundedCliph);
		bufferPainter.scale(mZoom,mZoom);
		bufferPainter.drawPixmap(0,0,
			mGVPixmap->pixmap(),
			int(roundedClipx/mZoom) - int(mXOffset/mZoom), int(roundedClipy/mZoom) - int(mYOffset/mZoom),
			int(roundedClipw/mZoom), int(roundedCliph/mZoom) );
	}

	painter->drawPixmap(clipx,clipy,buffer,clipx-roundedClipx,clipy-roundedClipy,clipw,cliph);

/*	painter->setPen(colors[numColor]);
	numColor=(numColor+1)%4;
	painter->drawRect(clipx,clipy,clipw,cliph);*/ // DEBUG Rects
}


void GVScrollPixmapView::viewportMousePressEvent(QMouseEvent* event) {
	if (event->button()==RightButton) return;
	
	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mDragStarted=true;
	setCursor(QCursor(SizeAllCursor));
}


void GVScrollPixmapView::viewportMouseMoveEvent(QMouseEvent* event) {
	if (!mDragStarted) return;

	int deltaX,deltaY;

	deltaX=mScrollStartX - event->x();
	deltaY=mScrollStartY - event->y();

	mScrollStartX=event->x();
	mScrollStartY=event->y();
	scrollBy(deltaX,deltaY);
}


void GVScrollPixmapView::viewportMouseReleaseEvent(QMouseEvent*) {
	if (mDragStarted) {
		unsetCursor();
		mDragStarted=false;
	}
}


//-Slots----------------------------------------------
void GVScrollPixmapView::slotZoomIn() {
	double newZoom;
	if (mZoom>=1.0) {
		newZoom=floor(mZoom)+1.0;
	} else {
		newZoom=1/( ceil(1/mZoom)-1.0 );
	}
	setZoom(newZoom);
}


void GVScrollPixmapView::slotZoomOut() {
	double newZoom;
	if (mZoom>1.0) {
		newZoom=ceil(mZoom)-1.0;
	} else {
		newZoom=1/( floor(1/mZoom)+1.0 );
	}
	setZoom(newZoom);
}


void GVScrollPixmapView::slotResetZoom() {
	setZoom(1.0);
}


void GVScrollPixmapView::setLockZoom(bool value) {
	mLockZoom=value;
}


//-Private---------------------------------------------
void GVScrollPixmapView::updateContentSize() {
	resizeContents(
		int(mGVPixmap->pixmap().width()*mZoom), 
		int(mGVPixmap->pixmap().height()*mZoom)	);
}


void GVScrollPixmapView::updateImageOffset(double oldZoom) {
	int viewWidth=visibleWidth();
	int viewHeight=visibleHeight();
	
	
	// Find the coordinate of the center of the image
	// and center the view on it
	int centerX=int( ((viewWidth/2+contentsX()-mXOffset)/oldZoom)*mZoom );
	int centerY=int( ((viewHeight/2+contentsY()-mYOffset)/oldZoom)*mZoom );
	center(centerX,centerY);

	// Compute mXOffset and mYOffset in case the image does not fit
	// the view width or height
	int zpixWidth=int(mGVPixmap->width() * mZoom);
	mXOffset=QMAX(0,(viewWidth-zpixWidth)/2);


	int zpixHeight=int(mGVPixmap->height() * mZoom);
	mYOffset=QMAX(0,(viewHeight-zpixHeight)/2);

}


//-Config----------------------------------------------
void GVScrollPixmapView::readConfig(KConfig*, const QString&) {
}


void GVScrollPixmapView::writeConfig(KConfig*, const QString&) const {
}

